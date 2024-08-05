/* MIT License
 *
 * Copyright (c) 2024 The c-ares project and its contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * SPDX-License-Identifier: MIT
 */

#ifdef __APPLE__

/* The DNS configuration for apple is stored in the system configuration
 * database.  Apple does provide an emulated `/etc/resolv.conf` on MacOS (but
 * not iOS), it cannot, however, represent the entirety of the DNS
 * configuration.  Alternatively, libresolv could be used to also retrieve some
 * system configuration, but it too is not capable of retrieving the entirety
 * of the DNS configuration.
 *
 * Attempts to use the preferred public API of `SCDynamicStoreCreate()` and
 * friends yielded incomplete DNS information.  Instead, that leaves some apple
 * "internal" symbols from `configd` that we need to access in order to get the
 * entire configuration.  We can see that we're not the only ones to do this as
 * Google Chrome also does:
 * https://chromium.googlesource.com/chromium/src/+/HEAD/net/dns/dns_config_watcher_mac.cc
 * These internal functions are what `libresolv` and `scutil` use to retrieve
 * the dns configuration.  Since these symbols are not publicly available, we
 * will dynamically load the symbols from `libSystem` and import the `dnsinfo.h`
 * private header extracted from:
 * https://opensource.apple.com/source/configd/configd-1109.140.1/dnsinfo/dnsinfo.h
 */
#  include "ares_setup.h"
#  include <stdio.h>
#  include <stdlib.h>
#  include <string.h>
#  include <dlfcn.h>
#  include <arpa/inet.h>
#  include "thirdparty/apple/dnsinfo.h"
#  include <SystemConfiguration/SCNetworkConfiguration.h>
#  include "ares.h"
#  include "ares_private.h"

typedef struct {
  void *handle;
  dns_config_t *(*dns_configuration_copy)(void);
  void (*dns_configuration_free)(dns_config_t *config);
} dnsinfo_t;

static void dnsinfo_destroy(dnsinfo_t *dnsinfo)
{
  if (dnsinfo == NULL) {
    return;
  }

  if (dnsinfo->handle) {
    dlclose(dnsinfo->handle);
  }

  ares_free(dnsinfo);
}

static ares_status_t dnsinfo_init(dnsinfo_t **dnsinfo_out)
{
  dnsinfo_t    *dnsinfo = NULL;
  ares_status_t status  = ARES_SUCCESS;

  if (dnsinfo_out == NULL) {
    status = ARES_EFORMERR;
    goto done;
  }

  *dnsinfo_out = NULL;

  dnsinfo = ares_malloc_zero(sizeof(*dnsinfo));

  if (dnsinfo == NULL) {
    status = ARES_ENOMEM;
    goto done;
  }

  dnsinfo->handle = dlopen("/usr/lib/libSystem.dylib", RTLD_LAZY | RTLD_NOLOAD);
  if (dnsinfo->handle == NULL) {
    status = ARES_ESERVFAIL;
    goto done;
  }

  dnsinfo->dns_configuration_copy =
    dlsym(dnsinfo->handle, "dns_configuration_copy");
  dnsinfo->dns_configuration_free =
    dlsym(dnsinfo->handle, "dns_configuration_free");

  if (dnsinfo->dns_configuration_copy == NULL ||
      dnsinfo->dns_configuration_free == NULL) {
    status = ARES_ESERVFAIL;
    goto done;
  }


done:
  if (status == ARES_SUCCESS) {
    *dnsinfo_out = dnsinfo;
  } else {
    dnsinfo_destroy(dnsinfo);
  }

  return status;
}

static ares_bool_t search_is_duplicate(const ares_sysconfig_t *sysconfig,
                                       const char             *name)
{
  size_t i;
  for (i = 0; i < sysconfig->ndomains; i++) {
    if (strcasecmp(sysconfig->domains[i], name) == 0) {
      return ARES_TRUE;
    }
  }
  return ARES_FALSE;
}

static ares_status_t read_resolver(const dns_resolver_t *resolver,
                                   ares_sysconfig_t     *sysconfig)
{
  int            i;
  unsigned short port   = 0;
  ares_status_t  status = ARES_SUCCESS;

  /* XXX: resolver->domain is for domain-specific servers.  When we implement
   *      this support, we'll want to use this.  But for now, we're going to
   *      skip any servers which set this since we can't properly route. */
  if (resolver->domain != NULL) {
    return ARES_SUCCESS;
  }

  /* Check to see if DNS server should be used, base this on if the server is
   * reachable or can be reachable automatically if we send traffic that
   * direction. */
  if (!(resolver->reach_flags &
        (kSCNetworkFlagsReachable |
         kSCNetworkReachabilityFlagsConnectionOnTraffic))) {
    return ARES_SUCCESS;
  }

  /* NOTE: it doesn't look like resolver->flags is relevant */

  /* If there's no nameservers, nothing to do */
  if (resolver->n_nameserver <= 0) {
    return ARES_SUCCESS;
  }

  /* Default port */
  port = resolver->port;

  /* Append search list */
  if (resolver->n_search > 0) {
    char **new_domains = ares_realloc_zero(
      sysconfig->domains, sizeof(*sysconfig->domains) * sysconfig->ndomains,
      sizeof(*sysconfig->domains) *
        (sysconfig->ndomains + (size_t)resolver->n_search));
    if (new_domains == NULL) {
      return ARES_ENOMEM;
    }
    sysconfig->domains = new_domains;

    for (i = 0; i < resolver->n_search; i++) {
      const char *search;
      /* UBSAN: copy pointer using memcpy due to misalignment */
      memcpy(&search, resolver->search + i, sizeof(search));

      /* Skip duplicates */
      if (search_is_duplicate(sysconfig, search)) {
        continue;
      }
      sysconfig->domains[sysconfig->ndomains] = ares_strdup(search);
      if (sysconfig->domains[sysconfig->ndomains] == NULL) {
        return ARES_ENOMEM;
      }
      sysconfig->ndomains++;
    }
  }

  /* NOTE: we're going to skip importing the sort addresses for now.  Its
   *       likely not used, its not obvious how to even configure such a thing.
   */
#  if 0
  for (i=0; i<resolver->n_sortaddr; i++) {
    char val[256];
    inet_ntop(AF_INET, &resolver->sortaddr[i]->address, val, sizeof(val));
    printf("\t\t%s/", val);
    inet_ntop(AF_INET, &resolver->sortaddr[i]->mask, val, sizeof(val));
    printf("%s\n", val);
  }
#  endif

  if (resolver->options != NULL) {
    status = ares__sysconfig_set_options(sysconfig, resolver->options);
    if (status != ARES_SUCCESS) {
      return status;
    }
  }

  /* NOTE:
   *   - resolver->timeout appears unused, always 0, so we ignore this
   *   - resolver->service_identifier doesn't appear relevant to us
   *   - resolver->cid also isn't relevant
   *   - resolver->if_index we don't need, if_name is used instead.
   */

  /* XXX: resolver->search_order appears like it might be relevant, we might
   * need to sort the resulting list by this metric if we find in the future we
   * need to.  That said, due to the automatic re-sorting we do, I'm not sure it
   * matters.  Here's an article on this search order stuff:
   *      https://www.cnet.com/tech/computing/os-x-10-6-3-and-dns-server-priority-changes/
   */

  for (i = 0; i < resolver->n_nameserver; i++) {
    struct ares_addr       addr;
    unsigned short         addrport;
    const struct sockaddr *sockaddr;

    /* UBSAN alignment workaround to fetch memory address */
    memcpy(&sockaddr, resolver->nameserver + i, sizeof(sockaddr));

    if (sockaddr->sa_family == AF_INET) {
      /* NOTE: memcpy sockaddr_in due to alignment issues found by UBSAN due to
       *       dnsinfo packing */
      struct sockaddr_in addr_in;
      memcpy(&addr_in, sockaddr, sizeof(addr_in));

      addr.family = AF_INET;
      memcpy(&addr.addr.addr4, &(addr_in.sin_addr), sizeof(addr.addr.addr4));
      addrport = ntohs(addr_in.sin_port);
    } else if (sockaddr->sa_family == AF_INET6) {
      /* NOTE: memcpy sockaddr_in6 due to alignment issues found by UBSAN due to
       *       dnsinfo packing */
      struct sockaddr_in6 addr_in6;
      memcpy(&addr_in6, sockaddr, sizeof(addr_in6));

      addr.family = AF_INET6;
      memcpy(&addr.addr.addr6, &(addr_in6.sin6_addr), sizeof(addr.addr.addr6));
      addrport = ntohs(addr_in6.sin6_port);
    } else {
      continue;
    }

    if (addrport == 0) {
      addrport = port;
    }
    status = ares__sconfig_append(&sysconfig->sconfig, &addr, addrport,
                                  addrport, resolver->if_name);
    if (status != ARES_SUCCESS) {
      return status;
    }
  }

  return status;
}

static ares_status_t read_resolvers(dns_resolver_t **resolvers, int nresolvers,
                                    ares_sysconfig_t *sysconfig)
{
  ares_status_t status = ARES_SUCCESS;
  int           i;

  for (i = 0; status == ARES_SUCCESS && i < nresolvers; i++) {
    const dns_resolver_t *resolver_ptr;
    dns_resolver_t        resolver;

    /* UBSAN doesn't like that this is unaligned, lets use memcpy to get the
     * address.  Equivalent to:
     *   resolver = resolvers[i]
     */
    memcpy(&resolver_ptr, resolvers + i, sizeof(resolver_ptr));

    /* UBSAN. If the pointer is misaligned, try to use memcpy to get the data
     * into a new structure that is hopefully aligned properly */
    memcpy(&resolver, resolver_ptr, sizeof(resolver));
    status = read_resolver(&resolver, sysconfig);
  }

  return status;
}

ares_status_t ares__init_sysconfig_macos(ares_sysconfig_t *sysconfig)
{
  dnsinfo_t    *dnsinfo = NULL;
  dns_config_t *sc_dns  = NULL;
  ares_status_t status  = ARES_SUCCESS;

  status = dnsinfo_init(&dnsinfo);

  if (status != ARES_SUCCESS) {
    goto done;
  }

  sc_dns = dnsinfo->dns_configuration_copy();
  if (sc_dns == NULL) {
    status = ARES_ESERVFAIL;
    goto done;
  }

  /* There are `resolver`, `scoped_resolver`, and `service_specific_resolver`
   * settings. The `scoped_resolver` settings appear to be already available via
   * the `resolver` settings and likely are only relevant to link-local dns
   * servers which we can already detect via the address itself, so we'll ignore
   * the `scoped_resolver` section.  It isn't clear what the
   * `service_specific_resolver` is used for, I haven't personally seen it
   * in use so we'll ignore this until at some point where we find we need it.
   * Likely this wasn't available via `/etc/resolv.conf` nor `libresolv` anyhow
   * so its not worse to prior configuration methods, worst case. */

  status = read_resolvers(sc_dns->resolver, sc_dns->n_resolver, sysconfig);

done:
  if (dnsinfo) {
    dnsinfo->dns_configuration_free(sc_dns);
    dnsinfo_destroy(dnsinfo);
  }
  return status;
}


#endif
