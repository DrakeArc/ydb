# CREATE RESOURCE POOL

`CREATE RESOURCE POOL` создаёт [пул ресурсов](../../../../concepts/gloassary#resource-pool.md).

## Синтаксис

```yql
CREATE RESOURCE POOL <name>
WITH ( <parameter_name> [= <parameter_value>] [, ... ] )
```

### Параметры {#paramters}
* `name` - имя создаваемого пула ресурсов. Должно быть уникально. Не допускается запись в виде пути (т.е. не должно содержать `/`).
* `WITH ( <parameter_name> [= <parameter_value>] [, ... ] )` позволяет задать значения параметров, определяющих поведение пула ресурсов. Поддерживаются следующие опции:
{% include [x](_includes/resource_pool_parameters.md) %}

## Замечания {#remark}

Запросы всегда выполняются в пуле ресурсов. По умолчанию все запросы отправляются в `default` пул ресурсов, который создается по умолчанию. Этот пул ресурсов нельзя удалить, он всегда существует в системе.

Если в `CONCURRENT_QUERY_LIMIT` установить значение `0`, то все запросы отправленные в этот пул ресурсов будут завершены незамедлительно со статусом `PRECONDITION_FAILED`

## Разрешения

Требуется [разрешение](../yql/reference/syntax/grant#permissions-list) `CREATE TABLE` на директорию `.metadata/workload_manager/pools`

Пример выдачи такого разрешения:
```yql
GRANT 'CREATE TABLE' ON `.metadata/workload_manager/pools` TO `user1@domain`;
```

## Примеры {#examples}

```yql
CREATE RESOURCE POOL olap WITH (
    CONCURRENT_QUERY_LIMIT=20,
    QUEUE_SIZE=1000,
    DATABASE_LOAD_CPU_THRESHOLD=80,
    RESOURCE_WEIGHT=100,
    QUERY_MEMORY_LIMIT_PERCENT_PER_NODE=80,
    TOTAL_CPU_LIMIT_PERCENT_PER_NODE=70
)
```

В примере выше создается пул ресурсов в котором есть ограничение на максимальное число параллельных запросов `20`, максимальный размер очереди ожидания `1000`. В случае достижения загрузки базы данных `80` процентов запросы перестают запускаться параллельно. Каждый запрос в пуле может потребить не больше `80` процентов доступной памяти на ноде, при достижении лимита потребления памяти запрос будет завершен со статусом `OVERLOADED`. Общее ограничение доступного CPU для всех запросов в пуле ресурсов на ноде составляет `70` процентов. Этот пул ресурсов имеет вес 100 и он начинает работать только в случае переподписки.

## См. также

* [Управление потреблением ресурсов](../../../dev/resource-pools-and-classifiers.md)
* [ALTER RESOURCE POOL](alter-resource-pool.md)
* [DROP RESOURCE POOL](drop-resource-pool.md)