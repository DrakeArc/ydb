#include "blobstorage_pdisk_ut.h"

#include "blobstorage_pdisk_abstract.h"
#include "blobstorage_pdisk_impl.h"
#include "blobstorage_pdisk_ut_env.h"

#include <ydb/core/blobstorage/crypto/default.h>
#include <ydb/core/testlib/actors/test_runtime.h>

#include <util/system/hp_timer.h>

namespace NKikimr {

Y_UNIT_TEST_SUITE(TPDiskTest) {
    Y_UNIT_TEST(TestAbstractPDiskInterface) {
        TString path = "/tmp/asdqwe";
        TIntrusivePtr<TPDiskConfig> cfg = new TPDiskConfig(path, 12345, 0xffffffffull,
                    TPDiskCategory(NPDisk::DEVICE_TYPE_ROT, 0).GetRaw());
        const TIntrusivePtr<::NMonitoring::TDynamicCounters> counters(new ::NMonitoring::TDynamicCounters);

        THolder<NPDisk::IPDisk> pDisk = MakeHolder<NPDisk::TPDisk>(cfg, counters);
        pDisk->Wakeup();
    }

    Y_UNIT_TEST(TestThatEveryValueOfEStateEnumKeepsItIntegerValue) {
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // Warning!
        // Kikimr Admins use Integer values of EState in their scripts!
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        UNIT_ASSERT(NKikimrBlobStorage::TPDiskState::Initial == 0);
        UNIT_ASSERT(NKikimrBlobStorage::TPDiskState::InitialFormatRead == 1);
        UNIT_ASSERT(NKikimrBlobStorage::TPDiskState::InitialFormatReadError == 2);
        UNIT_ASSERT(NKikimrBlobStorage::TPDiskState::InitialSysLogRead == 3);
        UNIT_ASSERT(NKikimrBlobStorage::TPDiskState::InitialSysLogReadError == 4);
        UNIT_ASSERT(NKikimrBlobStorage::TPDiskState::InitialSysLogParseError == 5);
        UNIT_ASSERT(NKikimrBlobStorage::TPDiskState::InitialCommonLogRead == 6);
        UNIT_ASSERT(NKikimrBlobStorage::TPDiskState::InitialCommonLogReadError == 7);
        UNIT_ASSERT(NKikimrBlobStorage::TPDiskState::InitialCommonLogParseError == 8);
        UNIT_ASSERT(NKikimrBlobStorage::TPDiskState::CommonLoggerInitError == 9);
        UNIT_ASSERT(NKikimrBlobStorage::TPDiskState::Normal == 10);
        UNIT_ASSERT(NKikimrBlobStorage::TPDiskState::OpenFileError == 11);
        UNIT_ASSERT(NKikimrBlobStorage::TPDiskState::ChunkQuotaError == 12);
        UNIT_ASSERT(NKikimrBlobStorage::TPDiskState::DeviceIoError == 13);
    }

    Y_UNIT_TEST(TestPDiskActorErrorState) {
        TActorTestContext testCtx(true);

        const TVDiskID vDiskID(0, 1, 0, 0, 0);
        testCtx.TestResponce<NPDisk::TEvYardInitResult>(
                new NPDisk::TEvYardInit(1, vDiskID, testCtx.TestCtx.PDiskGuid),
                NKikimrProto::CORRUPTED);
        testCtx.TestResponce<NPDisk::TEvCheckSpaceResult>(
                new NPDisk::TEvCheckSpace(1, 1),
                NKikimrProto::CORRUPTED);
        testCtx.TestResponce<NPDisk::TEvLogResult>(
                new NPDisk::TEvLog(1, 1, 0, nullptr, TLsnSeg(1, 1), nullptr),
                NKikimrProto::CORRUPTED);
        testCtx.TestResponce<NPDisk::TEvReadLogResult>(
                new NPDisk::TEvReadLog(1, 1, NPDisk::TLogPosition{0, 0}),
                NKikimrProto::CORRUPTED);

        testCtx.TestResponce<NPDisk::TEvChunkWriteResult>(
                new NPDisk::TEvChunkWrite(1, 1, 1, 0, nullptr, nullptr, false, 1),
                NKikimrProto::CORRUPTED);
        testCtx.TestResponce<NPDisk::TEvChunkReadResult>(
                new NPDisk::TEvChunkRead(1, 1, 17, 0, 4096, 1, nullptr),
                NKikimrProto::CORRUPTED);
        testCtx.TestResponce<NPDisk::TEvHarakiriResult>(
                new NPDisk::TEvHarakiri(1, 1),
                NKikimrProto::CORRUPTED);
        testCtx.TestResponce<NPDisk::TEvSlayResult>(
                new NPDisk::TEvSlay(vDiskID, 1, 1, 1),
                NKikimrProto::CORRUPTED);
        testCtx.TestResponce<NPDisk::TEvChunkReserveResult>(
                new NPDisk::TEvChunkReserve(1, 1, 3),
                NKikimrProto::CORRUPTED);
        testCtx.TestResponce<NPDisk::TEvYardControlResult>(
                new NPDisk::TEvYardControl(NPDisk::TEvYardControl::ActionPause, nullptr),
                NKikimrProto::CORRUPTED);

        testCtx.Send(new NActors::TEvents::TEvPoisonPill());
    }

    Y_UNIT_TEST(TestPDiskActorPDiskStopStart) {
        TActorTestContext testCtx(false);

        const TVDiskID vDiskID(0, 1, 0, 0, 0);
        testCtx.TestResponce<NPDisk::TEvYardInitResult>(
                new NPDisk::TEvYardInit(2, vDiskID, testCtx.TestCtx.PDiskGuid),
                NKikimrProto::OK);
        testCtx.TestResponce<NPDisk::TEvYardControlResult>(
                new NPDisk::TEvYardControl(NPDisk::TEvYardControl::PDiskStop, nullptr),
                NKikimrProto::OK);
        testCtx.TestResponce<NPDisk::TEvYardInitResult>(
                new NPDisk::TEvYardInit(3, vDiskID, testCtx.TestCtx.PDiskGuid),
                NKikimrProto::CORRUPTED);
        testCtx.TestResponce<NPDisk::TEvYardControlResult>(
                new NPDisk::TEvYardControl(NPDisk::TEvYardControl::PDiskStart, reinterpret_cast<void*>(&testCtx.MainKey)),
                NKikimrProto::OK);
        testCtx.TestResponce<NPDisk::TEvYardInitResult>(
                new NPDisk::TEvYardInit(3, vDiskID, testCtx.TestCtx.PDiskGuid),
                NKikimrProto::OK);

        testCtx.Send(new NActors::TEvents::TEvPoisonPill());
    }

    Y_UNIT_TEST(TestChunkWriteRelease) {
        for (ui32 i = 0; i < 16; ++i) {
            TestChunkWriteReleaseRun();
        }
    }

    Y_UNIT_TEST(TestPDiskOwnerRecreation) {
        TActorTestContext testCtx(false);

        const TVDiskID vDiskID(0, 1, 0, 0, 0);
        for (ui32 i = 2; i < 2000; ++i) {
            const auto evInitRes = testCtx.TestResponce<NPDisk::TEvYardInitResult>(
                    new NPDisk::TEvYardInit(i, vDiskID, testCtx.TestCtx.PDiskGuid),
                    NKikimrProto::OK);

            testCtx.TestResponce<NPDisk::TEvSlayResult>(
                    new NPDisk::TEvSlay(vDiskID, evInitRes->PDiskParams->OwnerRound + 1, 0, 0),
                    NKikimrProto::OK);
        }
    }

    Y_UNIT_TEST(TestPDiskOwnerRecreationWithStableOwner) {
        TActorTestContext testCtx(false);

        // Create "stable" owner, who will be alive during all test
        ui32 i = 2;
        const TVDiskID vDiskID_stable(0, 1, 0, 0, 0);
        testCtx.TestResponce<NPDisk::TEvYardInitResult>(
                new NPDisk::TEvYardInit(i++, vDiskID_stable, testCtx.TestCtx.PDiskGuid),
                NKikimrProto::OK);

        const TVDiskID vDiskID(1, 1, 0, 0, 0);
        for (; i < 2000; ++i) {
            const auto evInitRes = testCtx.TestResponce<NPDisk::TEvYardInitResult>(
                    new NPDisk::TEvYardInit(i, vDiskID, testCtx.TestCtx.PDiskGuid),
                    NKikimrProto::OK);

            testCtx.TestResponce<NPDisk::TEvSlayResult>(
                    new NPDisk::TEvSlay(vDiskID, evInitRes->PDiskParams->OwnerRound + 1, 0, 0),
                    NKikimrProto::OK);
        }
    }

    Y_UNIT_TEST(TestPDiskManyOwnersInitiation) {
        TActorTestContext testCtx(false);

        TVector<TVDiskIDOwnerRound> goodIds;
        ui64 badIdsCount = 0;

        for (int i = 2; i < 2000; ++i) {
            const TVDiskID vDiskID(i, 1, 0, 0, 0);
            if (badIdsCount == 0) {
                testCtx.Send(new NPDisk::TEvYardInit(i, vDiskID, testCtx.TestCtx.PDiskGuid));
                const auto evInitRes = testCtx.Recv<NPDisk::TEvYardInitResult>();
                if (evInitRes->Status == NKikimrProto::OK) {
                    goodIds.push_back({vDiskID, evInitRes->PDiskParams->OwnerRound});
                } else {
                    ++badIdsCount;
                }
            } else {
                const auto evInitRes = testCtx.TestResponce<NPDisk::TEvYardInitResult>(
                        new NPDisk::TEvYardInit(i, vDiskID, testCtx.TestCtx.PDiskGuid),
                        NKikimrProto::ERROR);
                ++badIdsCount;
            }
        }

        RecreateOwner(testCtx, goodIds.front());

        UNIT_ASSERT(badIdsCount > 0 && goodIds.size() > 0);
        for (auto v : goodIds) {
            testCtx.TestResponce<NPDisk::TEvSlayResult>(
                    new NPDisk::TEvSlay(v.VDiskID, v.OwnerRound + 1, 0, 0),
                    NKikimrProto::OK);
        }
    }

    Y_UNIT_TEST(TestVDiskMock) {
        TActorTestContext testCtx(false);
        TVDiskMock mock(&testCtx);

        mock.InitFull();
        const int logsSent = 100;
        for (int i = 0; i < logsSent; ++i) {
            mock.SendEvLogSync();
        }

        mock.Init();
        UNIT_ASSERT(mock.ReadLog() == mock.OwnedLogRecords());
    }

    // Test to reproduce bug from KIKIMR-10192
    Y_UNIT_TEST(TestLogSpliceNonceJump) {
        if constexpr (!KIKIMR_PDISK_ENABLE_CUT_LOG_FROM_THE_MIDDLE) {
            return;
        }

        TActorTestContext testCtx(false);
        TVDiskMock sporadicVDisk(&testCtx);
        TVDiskMock intensiveVDisk(&testCtx);

        sporadicVDisk.InitFull(); // writes log into logChunk# 1
        intensiveVDisk.InitFull();

        sporadicVDisk.SendEvLogSync();

        for (ui32 i = 0; i < 5; i++) {
            do {
                intensiveVDisk.SendEvLogSync(1024);
            } while (!testCtx.GetPDisk()->CommonLogger->OnFirstSectorInChunk());
        }
        // expect log chunks list looks like 1 -> 2 -> ... -> 6 (empty)

        testCtx.RestartPDiskSync(); // writes NonceJump
        UNIT_ASSERT_C(testCtx.GetPDisk()->CommonLogger->SectorIdx == 1, "To reproduce bug nonce jump record"
                " should be written in chunk's first sector");
        sporadicVDisk.InitFull(); // sends EvLog into chunk with recently written nonce jump
        intensiveVDisk.InitFull();

        {
            // initiate log splicing, expect transition to be
            // 1 -> 2 -> ... -> 6
            NPDisk::TPDisk *pdisk = testCtx.GetPDisk();
            UNIT_ASSERT(KIKIMR_PDISK_ENABLE_CUT_LOG_FROM_THE_MIDDLE);
            UNIT_ASSERT(pdisk->LogChunks.size() == 6);
            intensiveVDisk.CutLogAllButOne();
            // 1 -> 6
            pdisk->PDiskThread.StopSync();
            while (pdisk->LogChunks.size() != 2) {
                pdisk->Update();
            }
        }

        testCtx.RestartPDiskSync();
        intensiveVDisk.Init();
        UNIT_ASSERT_VALUES_EQUAL(intensiveVDisk.ReadLog(), intensiveVDisk.OwnedLogRecords());
        sporadicVDisk.Init();
        UNIT_ASSERT_VALUES_EQUAL(sporadicVDisk.ReadLog(), sporadicVDisk.OwnedLogRecords());

        testCtx.Send(new NActors::TEvents::TEvPoisonPill());
    }

    // Test to reproduce bug with multiple chunk splices from
    Y_UNIT_TEST(TestMultipleLogSpliceNonceJump) {
        if constexpr (!KIKIMR_PDISK_ENABLE_CUT_LOG_FROM_THE_MIDDLE) {
            return;
        }

        TActorTestContext testCtx(false);
        TVDiskMock sporadicVDisk(&testCtx);
        TVDiskMock moderateVDisk(&testCtx);
        TVDiskMock intensiveVDisk(&testCtx);

        sporadicVDisk.InitFull(); // writes log into logChunk# 1
        moderateVDisk.InitFull(); // writes log into logChunk# 1
        intensiveVDisk.InitFull();

        sporadicVDisk.SendEvLogSync();

        for (ui32 i = 0; i < 8; i++) {
            bool alreadyWriteThisChunk = false;
            do {
                if (1 <= i && i <= 4 && !alreadyWriteThisChunk) {
                    alreadyWriteThisChunk = true;
                    moderateVDisk.SendEvLogSync(1024);
                } else {
                    intensiveVDisk.SendEvLogSync(1024);
                }
            } while (!testCtx.GetPDisk()->CommonLogger->OnFirstSectorInChunk());
        }
        // expect log chunks list looks like 1 -> 2 -> ... -> 6 (empty)

        testCtx.RestartPDiskSync(); // writes NonceJump
        UNIT_ASSERT_C(testCtx.GetPDisk()->CommonLogger->SectorIdx == 1, "To reproduce bug nonce jump record"
                " should be written in chunk's first sector");
        sporadicVDisk.InitFull(); // sends EvLog into chunk with recently written nonce jump
        moderateVDisk.InitFull();
        intensiveVDisk.InitFull();

        {
            // initiate log splicing, expect transition to be
            // 1 -> 2 -> ... -> 9
            NPDisk::TPDisk *pdisk = testCtx.GetPDisk();
            UNIT_ASSERT(KIKIMR_PDISK_ENABLE_CUT_LOG_FROM_THE_MIDDLE);
            UNIT_ASSERT_C(pdisk->LogChunks.size() == 9, pdisk->LogChunks.size());
            intensiveVDisk.CutLogAllButOne();
            // 1 -> 2 -> 6
            pdisk->PDiskThread.StopSync();
            while (pdisk->LogChunks.size() != 6) {
                pdisk->Update();
            }
        }

        testCtx.RestartPDiskSync();
        moderateVDisk.Init();
        {
            // initiate log splicing, expect transition to be
            // 1 -> 2 -> ... -> 6
            NPDisk::TPDisk *pdisk = testCtx.GetPDisk();
            UNIT_ASSERT(KIKIMR_PDISK_ENABLE_CUT_LOG_FROM_THE_MIDDLE);
            UNIT_ASSERT(pdisk->LogChunks.size() == 6);
            moderateVDisk.CutLogAllButOne();
            // 1 -> 2 -> 6
            pdisk->PDiskThread.StopSync();
            while (pdisk->LogChunks.size() != 2) {
                pdisk->Update();
            }
        }

        testCtx.RestartPDiskSync();
        intensiveVDisk.Init();
        UNIT_ASSERT_VALUES_EQUAL(intensiveVDisk.ReadLog(), intensiveVDisk.OwnedLogRecords());
        moderateVDisk.Init();
        UNIT_ASSERT_VALUES_EQUAL(moderateVDisk.ReadLog(), moderateVDisk.OwnedLogRecords());
        sporadicVDisk.Init();
        UNIT_ASSERT_VALUES_EQUAL(sporadicVDisk.ReadLog(), sporadicVDisk.OwnedLogRecords());

        testCtx.Send(new NActors::TEvents::TEvPoisonPill());
    }

    Y_UNIT_TEST(TestFakeErrorPDiskManyLogWrite) {
        TActorTestContext testCtx(false);
        testCtx.TestCtx.SectorMap->ImitateIoErrorProbability = 1e-4;

        const TVDiskID vDiskID(0, 1, 0, 0, 0);
        const auto evInitRes = testCtx.TestResponce<NPDisk::TEvYardInitResult>(
                new NPDisk::TEvYardInit(2, vDiskID, testCtx.TestCtx.PDiskGuid),
                NKikimrProto::OK);

        ui32 errors = 0;
        ui32 lsn = 2;
        for (ui32 i = 0; i < 100'000; ++i) {
            testCtx.Send(new NPDisk::TEvLog(evInitRes->PDiskParams->Owner, evInitRes->PDiskParams->OwnerRound, 0,
                        TString("abc"), TLsnSeg(lsn, lsn), nullptr));
            ++lsn;
            const auto logRes = testCtx.Recv<NPDisk::TEvLogResult>();
            if (logRes->Status != NKikimrProto::OK) {
                ++errors;
            } else {
                UNIT_ASSERT(errors == 0);
            }
        }
        UNIT_ASSERT(errors > 0);
    }

    Y_UNIT_TEST(TestFakeErrorPDiskManyChunkRead) {
        TActorTestContext testCtx(false);
        testCtx.TestCtx.SectorMap->ImitateReadIoErrorProbability = 1e-4;

        TVDiskMock vdisk(&testCtx);
        vdisk.InitFull();

        ui32 errors = 0;

        vdisk.ReserveChunk();
        vdisk.CommitReservedChunks();
        UNIT_ASSERT(vdisk.Chunks[EChunkState::COMMITTED].size() == 1);
        const ui32 reservedChunk = *vdisk.Chunks[EChunkState::COMMITTED].begin();

        TString chunkWriteData = PrepareData(1024);
        testCtx.TestResponce<NPDisk::TEvChunkWriteResult>(
                new NPDisk::TEvChunkWrite(vdisk.PDiskParams->Owner, vdisk.PDiskParams->OwnerRound,
                    reservedChunk, 0, new NPDisk::TEvChunkWrite::TStrokaBackedUpParts(chunkWriteData), nullptr, false, 0),
                NKikimrProto::OK);

        bool printed = false;
        for (ui32 i = 0; i < 100'000; ++i) {
            testCtx.Send(new NPDisk::TEvChunkRead(vdisk.PDiskParams->Owner, vdisk.PDiskParams->OwnerRound,
                    reservedChunk, 0, 1024, 0, nullptr));

            const auto res = testCtx.Recv<NPDisk::TEvChunkReadResult>();
            //Ctest << res->ToString() << Endl;
            if (res->Status != NKikimrProto::OK) {
                ++errors;
                if (!printed) {
                    printed = true;
                    Ctest << res->ToString() << Endl;
                }
            }
        }
        // Check that PDisk is in working state now
        vdisk.InitFull();
    }

    Y_UNIT_TEST(TestFakeErrorPDiskManyChunkWrite) {
        TActorTestContext testCtx(false);
        testCtx.TestCtx.SectorMap->ImitateIoErrorProbability = 1e-4;

        const TVDiskID vDiskID(0, 1, 0, 0, 0);
        const auto evInitRes = testCtx.TestResponce<NPDisk::TEvYardInitResult>(
                new NPDisk::TEvYardInit(2, vDiskID, testCtx.TestCtx.PDiskGuid),
                NKikimrProto::OK);

        ui32 errors = 0;
        const auto evReserveRes = testCtx.TestResponce<NPDisk::TEvChunkReserveResult>(
                new NPDisk::TEvChunkReserve(evInitRes->PDiskParams->Owner, evInitRes->PDiskParams->OwnerRound, 1),
                NKikimrProto::OK);
        UNIT_ASSERT(evReserveRes->ChunkIds.size() == 1);
        const ui32 reservedChunk = evReserveRes->ChunkIds.front();

        bool printed = false;
        for (ui32 i = 0; i < 100'000; ++i) {
            TString data = PrepareData(1024);
            testCtx.Send(new NPDisk::TEvChunkWrite(evInitRes->PDiskParams->Owner, evInitRes->PDiskParams->OwnerRound,
                    reservedChunk, 0, new NPDisk::TEvChunkWrite::TStrokaBackedUpParts(data), nullptr, false, 0));

            const auto res = testCtx.Recv<NPDisk::TEvChunkWriteResult>();
            //Ctest << res->ToString() << Endl;
            if (res->Status != NKikimrProto::OK) {
                ++errors;
                if (!printed) {
                    printed = true;
                    Ctest << res->ToString() << Endl;
                }
            } else {
                UNIT_ASSERT(errors == 0);
            }
        }
        UNIT_ASSERT(errors > 0);
    }

    Y_UNIT_TEST(TestSIGSEGVInTUndelivered) {
        TActorTestContext testCtx(false);
        const TVDiskID vDiskID(0, 1, 0, 0, 0);
        const auto evInitRes = testCtx.TestResponce<NPDisk::TEvYardInitResult>(
                new NPDisk::TEvYardInit(2, vDiskID, testCtx.TestCtx.PDiskGuid),
                NKikimrProto::OK);

        TEvents::TEvUndelivered::TPtr ev = reinterpret_cast<TEventHandle<TEvents::TEvUndelivered>*>(
            new IEventHandle(
                testCtx.Sender, testCtx.Sender,
                new TEvents::TEvUndelivered(0, 0)
            )
        );

        const auto& sender = ev->Sender;
        THolder<NPDisk::TUndelivered> req{testCtx.GetPDisk()->ReqCreator.CreateFromEv<NPDisk::TUndelivered>(ev, sender)};
    }

    Y_UNIT_TEST(PDiskRestart) {
        TActorTestContext testCtx(false);
        TVDiskMock vdisk(&testCtx);
        vdisk.InitFull();
        vdisk.SendEvLogSync();
        testCtx.Send(new TEvBlobStorage::TEvRestartPDisk(testCtx.GetPDisk()->PDiskId, testCtx.MainKey, nullptr));
        const auto evInitRes = testCtx.Recv<TEvBlobStorage::TEvRestartPDiskResult>();
        vdisk.InitFull();
        vdisk.SendEvLogSync();
    }

    Y_UNIT_TEST(PDiskRestartManyLogWrites) {
        TActorTestContext testCtx(false);
        testCtx.TestCtx.SectorMap->ImitateIoErrorProbability = 1e-4;

        const TVDiskID vDiskID(0, 1, 0, 0, 0);
        const auto evInitRes = testCtx.TestResponce<NPDisk::TEvYardInitResult>(
                new NPDisk::TEvYardInit(2, vDiskID, testCtx.TestCtx.PDiskGuid),
                NKikimrProto::OK);

        //ui32 errors = 0;
        ui32 lsn = 2;
        TString logData = PrepareData(4096);
        for (ui32 i = 0; i < 1000; ++i) {
            testCtx.Send(new NPDisk::TEvLog(evInitRes->PDiskParams->Owner, evInitRes->PDiskParams->OwnerRound, 0,
                        logData, TLsnSeg(lsn, lsn), nullptr));
            if (i == 100) {
                testCtx.Send(new TEvBlobStorage::TEvRestartPDisk(testCtx.GetPDisk()->PDiskId, testCtx.MainKey, nullptr));
            }
            ++lsn;
        }
        for (ui32 i = 0; i < 100;) {
            const auto logRes = testCtx.Recv<NPDisk::TEvLogResult>();
            i += logRes->Results.size();
            if (logRes->Status == NKikimrProto::OK) {
                Ctest << "TEvLogResult status is ok" << Endl;
            } else {
                Ctest << "TEvLogResult status is error" << Endl;
            }
        }
        testCtx.Recv<TEvBlobStorage::TEvRestartPDiskResult>();
    }

    Y_UNIT_TEST(CommitDeleteChunks) {
        TActorTestContext testCtx(false);
        TVDiskMock intensiveVDisk(&testCtx);
        intensiveVDisk.InitFull();
        intensiveVDisk.ReserveChunk();
        intensiveVDisk.ReserveChunk();
        intensiveVDisk.CommitReservedChunks();
        intensiveVDisk.SendEvLogSync();
        intensiveVDisk.DeleteCommitedChunks();
        intensiveVDisk.InitFull();
    }

    // Test to reproduce bug from
    Y_UNIT_TEST(TestLogSpliceChunkReserve) {
        if constexpr (!KIKIMR_PDISK_ENABLE_CUT_LOG_FROM_THE_MIDDLE) {
            return;
        }

        TActorTestContext testCtx(false);
        TVDiskMock intensiveVDisk(&testCtx);    // idx# 1
        TVDiskMock formerVDisk(&testCtx);       // idx# 2
        TVDiskMock latterVDisk(&testCtx);       // idx# 3
        TVDiskMock fillerVDisk(&testCtx);

        // [1, 2, 3] -> [2] -> [1, 2, 3(+15)] -> [3(-15)] -> [1, 2, 3] -> ... -> [1, 2, 3]
        //
        // latter (owner 3) cuts log
        // [1, 2]  ->   [2] -> [1, 2]         *->            [1, 2]    -> ... -> [1, 2, 3]
        //
        // former (owner 2) cuts log
        // [1]     *->         [1]            *->            [1]       -> ... -> [1, 2, 3]

        intensiveVDisk.InitFull();
        formerVDisk.InitFull();
        latterVDisk.InitFull();
        fillerVDisk.InitFull();


        auto logChunks = [&] () {
            return testCtx.SafeRunOnPDisk([](NPDisk::TPDisk *pdisk) {
                return pdisk->LogChunks.size();
            });
        };

        const ui32 targetLogChunkCount = 9;
        for (ui32 i = 0; i < targetLogChunkCount - 1; i++) {
            if (i != 1 && i != 3) {
                intensiveVDisk.SendEvLogSync(16);
            }

            if (i != 3) {
                formerVDisk.SendEvLogSync(16);
            }

            if (i != 1) {
                latterVDisk.SendEvLogSync(16);
            }
            if (i == 2) {
                latterVDisk.ReserveChunk();
                latterVDisk.CommitReservedChunks();
            }
            if (i == 3) {
                latterVDisk.DeleteCommitedChunks();
            }

            do {
                fillerVDisk.SendEvLogSync(16);
            } while (logChunks() != i + 2);
        }

        // To remove obstructing owner in PDisk's Log prints
        fillerVDisk.CutLogAllButOne();

        {
            auto printLog = [&] () {
                testCtx.SafeRunOnPDisk([](NPDisk::TPDisk *pdisk) {
                    TStringStream out;
                    for (auto& info : pdisk->LogChunks) {
                        out << "[";
                        out << info.ChunkIdx << ": ";
                        for (size_t i = 0; i < info.OwnerLsnRange.size(); ++i) {
                            const NPDisk::TLogChunkInfo::TLsnRange &range = info.OwnerLsnRange[i];
                            if (range.IsPresent) {
                                out << i << ",";
                            }
                        }
                        out << "]";
                        out << " -> ";
                    }
                    out << Endl;
                    Ctest << out.Str();
                });
            };

            printLog();

            UNIT_ASSERT_C(logChunks() == targetLogChunkCount, "LogChunks.size()# " << logChunks());

            do {
                latterVDisk.CutLogAllButOne();
                printLog();
                Sleep(TDuration::Seconds(1));
            } while (logChunks() != targetLogChunkCount - 1);
            printLog();

            do {
                formerVDisk.CutLogAllButOne();
                printLog();
                Sleep(TDuration::Seconds(1));
            } while (logChunks() != targetLogChunkCount - 2);
            printLog();

            testCtx.RestartPDiskSync();
        }

        intensiveVDisk.InitFull();
        formerVDisk.InitFull();
        latterVDisk.InitFull();
    }

    Y_UNIT_TEST(SpaceColor) {
        return; // Enable test after KIKIMR-12880

        TActorTestContext testCtx(false);
        TVDiskMock vdisk(&testCtx);

        using TColor = NKikimrBlobStorage::TPDiskSpaceColor;
        for (auto color : {
                    TColor::GREEN,
                    TColor::CYAN,
                    TColor::LIGHT_YELLOW,
                    TColor::YELLOW,
                    TColor::LIGHT_ORANGE,
                    TColor::ORANGE,
                    TColor::RED,
                    //TColor::BLACK,
                } ){
            auto pdiskConfig = testCtx.GetPDiskConfig();
            pdiskConfig->SpaceColorBorder = color;
            pdiskConfig->ExpectedSlotCount = 10;
            testCtx.UpdateConfigRecreatePDisk(pdiskConfig);

            vdisk.InitFull();
            auto initialSpace = testCtx.TestResponce<NPDisk::TEvCheckSpaceResult>(
                    new NPDisk::TEvCheckSpace(vdisk.PDiskParams->Owner, vdisk.PDiskParams->OwnerRound),
                    NKikimrProto::OK);
            for (ui32 i = 0; i < initialSpace->FreeChunks + 1; ++i) {
                vdisk.ReserveChunk();
            }
            vdisk.CommitReservedChunks();
            auto resultSpace = testCtx.TestResponce<NPDisk::TEvCheckSpaceResult>(
                    new NPDisk::TEvCheckSpace(vdisk.PDiskParams->Owner, vdisk.PDiskParams->OwnerRound),
                    NKikimrProto::OK);
            UNIT_ASSERT(color == StatusFlagToSpaceColor(resultSpace->StatusFlags));
            vdisk.DeleteCommitedChunks();
        }
    }

    Y_UNIT_TEST(DeviceHaltTooLong) {
        TActorTestContext testCtx(false);
        testCtx.TestCtx.SectorMap->ImitateRandomWait = {TDuration::Seconds(1), TDuration::Seconds(2)};

        TVDiskMock mock(&testCtx);

        mock.InitFull();
        const int logsSent = 100;
        for (int i = 0; i < logsSent; ++i) {
            mock.SendEvLogSync();
        }

        mock.Init();
        UNIT_ASSERT(mock.ReadLog() == mock.OwnedLogRecords());
    }

    Y_UNIT_TEST(TestPDiskOnDifferentKeys) {
        TActorTestContext testCtx(false);

        int round = 2;
        const TVDiskID vDiskID(0, 1, 0, 0, 0);
        auto evInitRes = testCtx.TestResponce<NPDisk::TEvYardInitResult>(
                new NPDisk::TEvYardInit(round, vDiskID, testCtx.TestCtx.PDiskGuid),
                NKikimrProto::OK);
        testCtx.TestResponce<NPDisk::TEvCheckSpaceResult>(
                new NPDisk::TEvCheckSpace(evInitRes->PDiskParams->Owner, evInitRes->PDiskParams->OwnerRound),
                NKikimrProto::OK);
        round = evInitRes->PDiskParams->OwnerRound + 1;

        testCtx.MainKey[0] += 123;
        testCtx.UpdateConfigRecreatePDisk(testCtx.GetPDiskConfig());

        evInitRes = testCtx.TestResponce<NPDisk::TEvYardInitResult>(
                new NPDisk::TEvYardInit(round, vDiskID, testCtx.TestCtx.PDiskGuid),
                NKikimrProto::CORRUPTED);
        testCtx.TestResponce<NPDisk::TEvCheckSpaceResult>(
                new NPDisk::TEvCheckSpace(evInitRes->PDiskParams->Owner, evInitRes->PDiskParams->OwnerRound),
                NKikimrProto::CORRUPTED);
    }

    Y_UNIT_TEST(ChangePDiskKey) {
        const TString data = PrepareData(4096);

        TActorTestContext testCtx(false);
        
        TVDiskMock mock(&testCtx);
        mock.InitFull();

        mock.ReserveChunk();
        const ui32 chunk = *mock.Chunks[EChunkState::RESERVED].begin();

        auto readChunk = [&]() {
            auto evReadRes = testCtx.TestResponce<NPDisk::TEvChunkReadResult>(
                    new NPDisk::TEvChunkRead(mock.PDiskParams->Owner, mock.PDiskParams->OwnerRound,
                            chunk, 0, data.size(), 0, nullptr),
                    NKikimrProto::OK);
            UNIT_ASSERT_VALUES_EQUAL(evReadRes->Data.ToString(), data); 
        };

        TString dataCopy = data;
        testCtx.TestResponce<NPDisk::TEvChunkWriteResult>(new NPDisk::TEvChunkWrite(mock.PDiskParams->Owner, mock.PDiskParams->OwnerRound,
            chunk, 0, new NPDisk::TEvChunkWrite::TStrokaBackedUpParts(dataCopy), nullptr, false, 0), 
            NKikimrProto::OK);
        mock.CommitReservedChunks();

        readChunk();

        testCtx.MainKey.push_back(0xFull);
        testCtx.RestartPDiskSync();
        mock.InitFull();
        readChunk();

        testCtx.MainKey = { 0xFull };
        testCtx.RestartPDiskSync();
        mock.InitFull();
        readChunk();

        testCtx.MainKey = { 0xFull, 0xA };
        testCtx.RestartPDiskSync();
        mock.InitFull();
        readChunk();

        testCtx.MainKey = { 0xFull, 0xA, 0xB, 0xC };
        testCtx.RestartPDiskSync();
        mock.InitFull();
        readChunk();

        testCtx.MainKey = { 0xC };
        testCtx.RestartPDiskSync();
        mock.InitFull();
        readChunk();
    }


    Y_UNIT_TEST(WrongPDiskKey) {
        const TString data = PrepareData(4096);

        TActorTestContext testCtx(false);
        
        TVDiskMock mock(&testCtx);
        mock.InitFull();

        mock.ReserveChunk();
        const ui32 chunk = *mock.Chunks[EChunkState::RESERVED].begin();

        TString dataCopy = data;
        testCtx.TestResponce<NPDisk::TEvChunkWriteResult>(new NPDisk::TEvChunkWrite(mock.PDiskParams->Owner, mock.PDiskParams->OwnerRound,
            chunk, 0, new NPDisk::TEvChunkWrite::TStrokaBackedUpParts(dataCopy), nullptr, false, 0), 
            NKikimrProto::OK);
        mock.CommitReservedChunks();
        testCtx.TestResponce<NPDisk::TEvCheckSpaceResult>(
                new NPDisk::TEvCheckSpace(mock.PDiskParams->Owner, mock.PDiskParams->OwnerRound),
                NKikimrProto::OK);
        testCtx.TestResponce<NPDisk::TEvChunkReadResult>(
                new NPDisk::TEvChunkRead(mock.PDiskParams->Owner, mock.PDiskParams->OwnerRound,
                        chunk, 0, data.size(), 0, nullptr),
                NKikimrProto::OK);

        testCtx.MainKey = { 0xABCDEF };
        testCtx.TestResponce<NPDisk::TEvYardControlResult>(
                new NPDisk::TEvYardControl(NPDisk::TEvYardControl::PDiskStop, nullptr),
                NKikimrProto::OK);
            
        testCtx.TestResponce<NPDisk::TEvYardControlResult>(
                new NPDisk::TEvYardControl(NPDisk::TEvYardControl::PDiskStart, (void*)(&testCtx.MainKey)),
                NKikimrProto::CORRUPTED);
    }
}
} // namespace NKikimr
