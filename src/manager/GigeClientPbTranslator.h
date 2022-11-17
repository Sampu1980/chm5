/**************************************************************************
   Copyright (c) 2020 Infinera
**************************************************************************/
#ifndef GIGE_CLIENT_PB_TRANSLATOR_H_
#define GIGE_CLIENT_PB_TRANSLATOR_H_

#include <string>

#include <DcoGigeClientAdapter.h>
#include <DpPm.h>

#include "DpProtoDefs.h"
#include "DpCommonPbTranslator.h"

#include "BcmPm.h"


namespace DataPlane {

class GigeClientPbTranslator : public DpCommonPbTranslator
{
public:
    GigeClientPbTranslator() {}

    ~GigeClientPbTranslator() {}

    std::string numToAid(uint gigeNum, std::string &tpAid) const;

    void configPbToAd(const chm6_dataplane::Chm6GigeClientConfig& pbCfg, DpAdapter::GigeCommon& adCfg) const;
    void configAdToPb(const DpAdapter::GigeCommon& adCfg, chm6_dataplane::Chm6GigeClientState& pbStat, bool isGbFecEnable) const;
    void statusAdToPb(const DpAdapter::GigeStatus& adStat, chm6_dataplane::Chm6GigeClientState& pbStat) const;
    void faultAdToPb(std::string aid, const uint64_t& faultBmp, const DpAdapter::GigeFault& adFault, chm6_dataplane::Chm6GigeClientFault& pbFault, bool force) const;
    bool isHardFault(const DpAdapter::GigeFault& adFault, DataPlane::FaultDirection direction) const;
    void faultAdGearBoxUpdate(const DpAdapter::GigeFault& adGearBoxFault, DpAdapter::GigeFault& adFault) const;
    void faultAdToPbSim(const uint64_t& fault, chm6_dataplane::Chm6GigeClientFault& pbFault) const;
    void pmAdToPb(const DataPlane::DpMsClientGigePmContainer& adPm, const gearbox::BcmPm& adGearBoxPm, bool testSignalMon, bool isHardFaultRx, bool isHardFaultTx, bool validity, chm6_dataplane::Chm6GigeClientPm & pbPm);
    void pmAdDefault(DataPlane::DpMsClientGigePmContainer& adPm, gearbox::BcmPm& adGearBoxPm);
    bool isPbChanged(const chm6_dataplane::Chm6GigeClientState& pbStatPrev, const chm6_dataplane::Chm6GigeClientState& pbStatCur);
    bool isAdChanged(const DpAdapter::GigeFault& adFaultPrev, const DpAdapter::GigeFault& adFaultCur);
    double calcFecSER(string aid, uint64_t fec_symbol_error);
    static void cacheFecDegSerMonPeriod(string aid, uint64_t monPer);

    struct FecSerCache
    {
        uint cachedMonPer;
        unsigned int monPerCounter;
        std::vector<uint64_t> fecSymErrs;

        FecSerCache() : cachedMonPer(1), monPerCounter(0) { }

    };

    static std::map<string, FecSerCache> mFecSerCache;
    static const unsigned int cMaxMonPer = 50;


private:

    enum GIGE_FAULT_PARAMS
    {
        GIGE_FAULT_LF_EGRESS        = 0,
        GIGE_FAULT_LF_INGRESS          ,
        GIGE_FAULT_LINE_TESTSIG_GEN    ,
        GIGE_FAULT_LINE_TESTSIG_OOS    ,
        GIGE_FAULT_LOA_EGRESS          ,
        GIGE_FAULT_LOA_INGRESS         ,
        GIGE_FAULT_LOLM_EGRESS         ,
        GIGE_FAULT_LOLM_INGRESS        ,
        GIGE_FAULT_HIBER_EGRESS        ,
        GIGE_FAULT_HIBER_INGRESS       ,
        GIGE_FAULT_LOSYNC_EGRESS       ,
        GIGE_FAULT_LOSYNC_INGRESS      ,
        GIGE_FAULT_OOS_AU              ,
        GIGE_FAULT_RF_EGRESS           ,
        GIGE_FAULT_RF_INGRESS          ,
        GIGE_TRIB_TESTSIG_GEN          ,
        GIGE_TRIB_TESTSIG_OOS          ,
        GIGE_FAULT_LOCAL_DEGRADED      ,
        GIGE_FAULT_REMOTE_DEGRADED     ,
        GIGE_FAULT_FEC_DEG_SER         ,
        NUM_GIGE_FAULT_PARAMS
    };

    const boost::unordered_map<GIGE_FAULT_PARAMS,const char*> gigeFaultParamsToString = boost::assign::map_list_of
        (GIGE_FAULT_LF_EGRESS       ,	"LF"              )
        (GIGE_FAULT_LF_INGRESS      ,	"LF"              )
        (GIGE_FAULT_LINE_TESTSIG_GEN,	"LINE-TESTSIG-GEN")
        (GIGE_FAULT_LINE_TESTSIG_OOS,	"LINE-TESTSIG-OOS")
        (GIGE_FAULT_LOA_EGRESS      ,	"LOA"             )
        (GIGE_FAULT_LOA_INGRESS     ,	"LOA"             )
        (GIGE_FAULT_LOLM_EGRESS     ,	"LOLM"            )
        (GIGE_FAULT_LOLM_INGRESS    ,	"LOLM"            )
        (GIGE_FAULT_HIBER_EGRESS      ,	"HIBER"           )
        (GIGE_FAULT_HIBER_INGRESS     ,	"HIBER"           )
        (GIGE_FAULT_LOSYNC_EGRESS   ,	"LOSYNC"          )
        (GIGE_FAULT_LOSYNC_INGRESS  ,	"LOSYNC"          )
        (GIGE_FAULT_OOS_AU          ,	"OOS-AU"          )
        (GIGE_FAULT_RF_EGRESS       ,	"RF"              )
        (GIGE_FAULT_RF_INGRESS      ,	"RF"              )
        (GIGE_TRIB_TESTSIG_GEN      ,	"TRIB-TESTSIG-GEN")
        (GIGE_TRIB_TESTSIG_OOS      ,	"TRIB-TESTSIG-OOS")
        (GIGE_FAULT_LOCAL_DEGRADED  ,   "LOCAL-DEGRADED"  )
        (GIGE_FAULT_REMOTE_DEGRADED ,   "REMOTE-DEGRADED" )
        (GIGE_FAULT_FEC_DEG_SER     ,   "FEC-DEGRADED-SER");

    void addGigeClientFaultPair(std::string faultName, DataPlane::FaultDirection faultDirection, DataPlane::FaultLocation faultLocation, bool value, chm6_dataplane::Chm6GigeClientFault& pbFault) const;

    enum GIGE_PM_PARAMS
    {
        GIGE_PM_PACKETS_INGRESS        = 0,
        GIGE_PM_OCTETS_INGRESS            ,
        GIGE_PM_ERR_PACKETS_INGRESS       ,
        GIGE_PM_ERR_OCTETS_INGRESS        ,
        GIGE_PM_JABBERS_INGRESS           ,
        GIGE_PM_FRAGMENTS_INGRESS         ,
        GIGE_PM_CRC_ALIGNED_ERR_INGRESS   ,
        GIGE_PM_UNDERSIZED_INGRESS        ,
        GIGE_PM_OVERSIZED_INGRESS         ,
        GIGE_PM_BROADCAST_PKTS_INGRESS    ,
        GIGE_PM_MULTICAST_PKTS_INGRESS    ,
        GIGE_PM_IN_PAUSE_FRAMES_INGRESS   ,
        GIGE_PM_PACKETS_EGRESS            ,
        GIGE_PM_OCTETS_EGRESS             ,
        GIGE_PM_ERR_PACKETS_EGRESS        ,
        GIGE_PM_ERR_OCTETS_EGRESS         ,
        GIGE_PM_JABBERS_EGRESS            ,
        GIGE_PM_FRAGMENTS_EGRESS          ,
        GIGE_PM_CRC_ALIGNED_ERR_EGRESS    ,
        GIGE_PM_UNDERSIZED_EGRESS         ,
        GIGE_PM_OVERSIZED_EGRESS          ,
        GIGE_PM_BROADCAST_PKTS_EGRESS     ,
        GIGE_PM_MULTICAST_PKTS_EGRESS     ,
        GIGE_PM_OUT_PAUSE_FRAMES_EGRESS   ,
        GIGE_PM_SIZE_64_INGRESS           ,
        GIGE_PM_SIZE_65_TO_127_INGRESS    ,
        GIGE_PM_SIZE_128_TO_255_INGRESS   ,
        GIGE_PM_SIZE_256_TO_511_INGRESS   ,
        GIGE_PM_SIZE_512_TO_1023_INGRESS  ,
        GIGE_PM_SIZE_1024_TO_1518_INGRESS ,
        GIGE_PM_SIZE_1519_TO_JUMBO_INGRESS,
        GIGE_PM_SIZE_64_EGRESS            ,
        GIGE_PM_SIZE_65_TO_127_EGRESS     ,
        GIGE_PM_SIZE_128_TO_255_EGRESS    ,
        GIGE_PM_SIZE_256_TO_511_EGRESS    ,
        GIGE_PM_SIZE_512_TO_1023_EGRESS   ,
        GIGE_PM_SIZE_1024_TO_1518_EGRESS  ,
        GIGE_PM_SIZE_1519_TO_JUMBO_EGRESS ,
        GIGE_PM_PCS_ICG_INGRESS           ,
        GIGE_PM_PCS_ICG_EGRESS            ,
        GIGE_PM_CORR_WORD_INGRESS         ,
        GIGE_PM_CORRECTED_BIT_INGRESS     ,
        GIGE_PM_UN_CORR_WORD_INGRESS      ,
        GIGE_PM_FEC_SYMBOL_ERROR_INGRESS  ,
        GIGE_PM_PCS_BIP_TOTAL_INGRESS     ,
        GIGE_PM_PRE_FEC_BER_INGRESS       ,
        GIGE_PM_FSE_RATE_INGRESS          ,
        GIGE_PM_TRIB_TEST_SIG_ERR         ,
        NUM_GIGE_PM_PARAMS
    };

    const boost::unordered_map<GIGE_PM_PARAMS,const char*> gigePmParamsToString = boost::assign::map_list_of
        (GIGE_PM_PACKETS_INGRESS           , "packets"               )
        (GIGE_PM_OCTETS_INGRESS            , "octets"                )
        (GIGE_PM_ERR_PACKETS_INGRESS       , "err-packets"           )
        (GIGE_PM_ERR_OCTETS_INGRESS        , "err-octets"            )
        (GIGE_PM_JABBERS_INGRESS           , "jabbers"               )
        (GIGE_PM_FRAGMENTS_INGRESS         , "fragments"             )
        (GIGE_PM_CRC_ALIGNED_ERR_INGRESS   , "crc-aligned-err"       )
        (GIGE_PM_UNDERSIZED_INGRESS        , "undersized"            )
        (GIGE_PM_OVERSIZED_INGRESS         , "oversized"             )
        (GIGE_PM_BROADCAST_PKTS_INGRESS    , "broadcast-pkts"        )
        (GIGE_PM_MULTICAST_PKTS_INGRESS    , "multicast-pkts"        )
        (GIGE_PM_IN_PAUSE_FRAMES_INGRESS   , "in-pause-frames"       )
        (GIGE_PM_PACKETS_EGRESS            , "packets"               )
        (GIGE_PM_OCTETS_EGRESS             , "octets"                )
        (GIGE_PM_ERR_PACKETS_EGRESS        , "err-packets"           )
        (GIGE_PM_ERR_OCTETS_EGRESS         , "err-octets"            )
        (GIGE_PM_JABBERS_EGRESS            , "jabbers"               )
        (GIGE_PM_FRAGMENTS_EGRESS          , "fragments"             )
        (GIGE_PM_CRC_ALIGNED_ERR_EGRESS    , "crc-aligned-err"       )
        (GIGE_PM_UNDERSIZED_EGRESS         , "undersized"            )
        (GIGE_PM_OVERSIZED_EGRESS          , "oversized"             )
        (GIGE_PM_BROADCAST_PKTS_EGRESS     , "broadcast-pkts"        )
        (GIGE_PM_MULTICAST_PKTS_EGRESS     , "multicast-pkts"        )
        (GIGE_PM_OUT_PAUSE_FRAMES_EGRESS   , "out-pause-frames"      )
        (GIGE_PM_SIZE_64_INGRESS           , "size-64"               )
        (GIGE_PM_SIZE_65_TO_127_INGRESS    , "size-65-to-127"        )
        (GIGE_PM_SIZE_128_TO_255_INGRESS   , "size-128-to-255"       )
        (GIGE_PM_SIZE_256_TO_511_INGRESS   , "size-256-to-511"       )
        (GIGE_PM_SIZE_512_TO_1023_INGRESS  , "size-512-to-1023"      )
        (GIGE_PM_SIZE_1024_TO_1518_INGRESS , "size-1024-to-1518"     )
        (GIGE_PM_SIZE_1519_TO_JUMBO_INGRESS, "size-1519-to-jumbo"    )
        (GIGE_PM_SIZE_64_EGRESS            , "size-64"               )
        (GIGE_PM_SIZE_65_TO_127_EGRESS     , "size-65-to-127"        )
        (GIGE_PM_SIZE_128_TO_255_EGRESS    , "size-128-to-255"       )
        (GIGE_PM_SIZE_256_TO_511_EGRESS    , "size-256-to-511"       )
        (GIGE_PM_SIZE_512_TO_1023_EGRESS   , "size-512-to-1023"      )
        (GIGE_PM_SIZE_1024_TO_1518_EGRESS  , "size-1024-to-1518"     )
        (GIGE_PM_SIZE_1519_TO_JUMBO_EGRESS , "size-1519-to-jumbo"    )
        (GIGE_PM_PCS_ICG_INGRESS           , "pcs-errored-blocks"    )
        (GIGE_PM_PCS_ICG_EGRESS            , "pcs-errored-blocks"    )
        (GIGE_PM_CORR_WORD_INGRESS         , "corrected-words"       )
        (GIGE_PM_UN_CORR_WORD_INGRESS      , "uncorrected-words"     )
        (GIGE_PM_CORRECTED_BIT_INGRESS     , "fec-corrected-bits"    )
        (GIGE_PM_FEC_SYMBOL_ERROR_INGRESS  , "fec-symbol-error"      )
        (GIGE_PM_PCS_BIP_TOTAL_INGRESS     , "pcs-bip-total"         )
        (GIGE_PM_PRE_FEC_BER_INGRESS       , "pre-fec-ber"           )
        (GIGE_PM_FSE_RATE_INGRESS          , "fec-symbol-error-rate" )
        (GIGE_PM_TRIB_TEST_SIG_ERR         , "trib-test-sig-err"     );

    const std::map<uint, std::string> mTomIdMap = boost::assign::map_list_of
            (1, "1.1")
            (2, "1.2")
            (3, "1.3")
            (4, "1.4")
            (5, "8.1")
            (6, "8.2")
            (7, "8.3")
            (8, "8.4")
            (9, "9.1")
            (10, "9.2")
            (11, "9.3")
            (12, "9.4")
            (13, "16.1")
            (14, "16.2")
            (15, "16.3")
            (16, "16.4");

    void addGigeClientPmPair(GIGE_PM_PARAMS pmParam, DataPlane::PmDirection pmDirection, DataPlane::PmLocation pmLocation, uint64_t value, bool validity, chm6_dataplane::Chm6GigeClientPm& pbPm) const;
    void addGigeClientPmPair(GIGE_PM_PARAMS pmParam, DataPlane::PmDirection pmDirection, DataPlane::PmLocation pmLocation, double value, bool validity, chm6_dataplane::Chm6GigeClientPm& pbPm) const;


};



}; // namespace DataPlane

#endif /* GIGE_CLIENT_PB_TRANSLATOR_H_ */
