#ifndef CHM6_DATAPLANE_MS_SRC_SIM_SIMDCOCARDCAPABILITIES_H_
#define CHM6_DATAPLANE_MS_SRC_SIM_SIMDCOCARDCAPABILITIES_H_

#include <infinera_dco_card/infinera_dco_card.pb.h>

using DcoCard_Capabilities = ::dcoyang::infinera_dco_card::DcoCard_Capabilities;
using cardLineMode = ::dcoyang::infinera_dco_card::DcoCard_Capabilities_SupportedLineModes;

namespace  dco_card = ::dcoyang::infinera_dco_card;

namespace DpSim
{

const std::vector<dco_card::DcoCard_Capabilities_SupportedClients> supportedClientVal =
{
        dco_card::DcoCard_Capabilities_SupportedClients_SUPPORTEDCLIENTS_100GB_ELAN,
        dco_card::DcoCard_Capabilities_SupportedClients_SUPPORTEDCLIENTS_400GB_ELAN,
};

struct SupportedLineMode
{
    std::string   lineMode;
    int           dataRate; // Capacity
    dco_card::DcoCard_Capabilities_SupportedLineModes_ClientMode clientMode;
    long long int baudRateDigits;
    int           baudRatePrecision;
    std::string   appCode;
    int           compId;
    dco_card::DcoCard_Capabilities_SupportedLineModes_CarrierModeStatus carrierModeStatus;
    int           maxPower;


    SupportedLineMode(char* lm, int dr, int cm, long long int brd, int brp, char* ac, int ci, int cms, int mp)
      : lineMode(lm)
      , dataRate(dr)
      , clientMode(static_cast<dco_card::DcoCard_Capabilities_SupportedLineModes_ClientMode>(cm))
      , baudRateDigits(brd)
      , baudRatePrecision(brp)
      , appCode(ac)
      , compId(ci)
      , carrierModeStatus(static_cast<dco_card::DcoCard_Capabilities_SupportedLineModes_CarrierModeStatus>(cms))
      , maxPower(mp)
    {}
};

const std::vector<SupportedLineMode> supportLineVal =
{
        SupportedLineMode((char*)"800E.96P"   , 800, cardLineMode::CLIENTMODE_LXTF20E, 956390657, 7, (char*)"P"   , 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"800E.96P_DL", 800, cardLineMode::CLIENTMODE_LXTF20E, 956390657, 7, (char*)"P_DL", 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"800E.96P_ML", 800, cardLineMode::CLIENTMODE_LXTF20E, 956390657, 7, (char*)"P_ML", 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"600E.96P"   , 600, cardLineMode::CLIENTMODE_LXTF20E, 956390657, 7, (char*)"P"   , 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"600E.96P_DL", 600, cardLineMode::CLIENTMODE_LXTF20E, 956390657, 7, (char*)"P_DL", 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"600E.96P_ML", 600, cardLineMode::CLIENTMODE_LXTF20E, 956390657, 7, (char*)"P_ML", 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"400E.96P"   , 400, cardLineMode::CLIENTMODE_LXTF20E, 956390657, 7, (char*)"P"   , 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"400E.96P_DL", 400, cardLineMode::CLIENTMODE_LXTF20E, 956390657, 7, (char*)"P_DL", 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"800E.91P_DL", 800, cardLineMode::CLIENTMODE_LXTF20E, 912918355, 7, (char*)"P_DL", 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"800E.91P_ML", 800, cardLineMode::CLIENTMODE_LXTF20E, 912918355, 7, (char*)"P_ML", 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"600E.91P"   , 600, cardLineMode::CLIENTMODE_LXTF20E, 912918355, 7, (char*)"P"   , 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"600E.91P_DL", 600, cardLineMode::CLIENTMODE_LXTF20E, 912918355, 7, (char*)"P_DL", 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"600E.91P_ML", 600, cardLineMode::CLIENTMODE_LXTF20E, 912918355, 7, (char*)"P_ML", 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"400E.91P"   , 400, cardLineMode::CLIENTMODE_LXTF20E, 912918355, 7, (char*)"P"   , 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"400E.91P_DL", 400, cardLineMode::CLIENTMODE_LXTF20E, 912918355, 7, (char*)"P_DL", 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"400E.91P_ML", 400, cardLineMode::CLIENTMODE_LXTF20E, 912918355, 7, (char*)"P_ML", 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"400E.91S"   , 400, cardLineMode::CLIENTMODE_LXTF20E, 912918355, 7, (char*)"S"   , 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"400E.91S_DL", 400, cardLineMode::CLIENTMODE_LXTF20E, 912918355, 7, (char*)"S_DL", 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"400E.91S_ML", 400, cardLineMode::CLIENTMODE_LXTF20E, 912918355, 7, (char*)"S_ML", 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"500E.91S"   , 500, cardLineMode::CLIENTMODE_LXTF20E, 912918355, 7, (char*)"S"   , 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"500E.91S_DL", 500, cardLineMode::CLIENTMODE_LXTF20E, 912918355, 7, (char*)"S_DL", 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"500E.91S_ML", 500, cardLineMode::CLIENTMODE_LXTF20E, 912918355, 7, (char*)"S_ML", 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"600E.91S"   , 600, cardLineMode::CLIENTMODE_LXTF20E, 912918355, 7, (char*)"S"   , 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"600E.91S_DL", 600, cardLineMode::CLIENTMODE_LXTF20E, 912918355, 7, (char*)"S_DL", 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"600E.91S_ML", 600, cardLineMode::CLIENTMODE_LXTF20E, 912918355, 7, (char*)"S_ML", 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"700E.91S"   , 700, cardLineMode::CLIENTMODE_LXTF20E, 912918355, 7, (char*)"S"   , 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"700E.91S_DL", 700, cardLineMode::CLIENTMODE_LXTF20E, 912918355, 7, (char*)"S_DL", 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"700E.91S_ML", 700, cardLineMode::CLIENTMODE_LXTF20E, 912918355, 7, (char*)"S_ML", 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"800E.91S"   , 800, cardLineMode::CLIENTMODE_LXTF20E, 912918355, 7, (char*)"S"   , 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"800E.91S_DL", 800, cardLineMode::CLIENTMODE_LXTF20E, 912918355, 7, (char*)"S_DL", 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"800E.91S_ML", 800, cardLineMode::CLIENTMODE_LXTF20E, 912918355, 7, (char*)"S_ML", 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"400E.96S"   , 400, cardLineMode::CLIENTMODE_LXTF20E, 956390657, 7, (char*)"S"   , 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"400E.96S_DL", 400, cardLineMode::CLIENTMODE_LXTF20E, 956390657, 7, (char*)"S_DL", 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"400E.96S_ML", 400, cardLineMode::CLIENTMODE_LXTF20E, 956390657, 7, (char*)"S_ML", 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"600E.96S"   , 600, cardLineMode::CLIENTMODE_LXTF20E, 956390657, 7, (char*)"S"   , 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"600E.96S_DL", 600, cardLineMode::CLIENTMODE_LXTF20E, 956390657, 7, (char*)"S_DL", 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"600E.96S_ML", 600, cardLineMode::CLIENTMODE_LXTF20E, 956390657, 7, (char*)"S_ML", 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"800E.96S"   , 800, cardLineMode::CLIENTMODE_LXTF20E, 956390657, 7, (char*)"S"   , 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"800E.96S_DL", 800, cardLineMode::CLIENTMODE_LXTF20E, 956390657, 7, (char*)"S_DL", 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"800E.96S_ML", 800, cardLineMode::CLIENTMODE_LXTF20E, 956390657, 7, (char*)"S_ML", 1, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"400E.72S"   , 400, cardLineMode::CLIENTMODE_LXTF20E, 717292992, 7, (char*)"S"   , 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"400E.72S_DL", 400, cardLineMode::CLIENTMODE_LXTF20E, 717292992, 7, (char*)"S_DL", 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"400E.72S_ML", 400, cardLineMode::CLIENTMODE_LXTF20E, 717292992, 7, (char*)"S_ML", 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"350E.72S"   , 350, cardLineMode::CLIENTMODE_LXTF20E, 717292992, 7, (char*)"S"   , 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"350E.72S_DL", 350, cardLineMode::CLIENTMODE_LXTF20E, 717292992, 7, (char*)"S_DL", 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"350E.72S_ML", 350, cardLineMode::CLIENTMODE_LXTF20E, 717292992, 7, (char*)"S_ML", 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"300E.72S"   , 300, cardLineMode::CLIENTMODE_LXTF20E, 717292992, 7, (char*)"S"   , 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"300E.72S_DL", 300, cardLineMode::CLIENTMODE_LXTF20E, 717292992, 7, (char*)"S_DL", 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"300E.72S_ML", 300, cardLineMode::CLIENTMODE_LXTF20E, 717292992, 7, (char*)"S_ML", 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"250E.72S"   , 250, cardLineMode::CLIENTMODE_LXTF20E, 717292992, 7, (char*)"S"   , 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"250E.72S_DL", 250, cardLineMode::CLIENTMODE_LXTF20E, 717292992, 7, (char*)"S_DL", 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"250E.72S_ML", 250, cardLineMode::CLIENTMODE_LXTF20E, 717292992, 7, (char*)"S_ML", 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"600E.84P"   , 600, cardLineMode::CLIENTMODE_LXTF20E, 836841825, 7, (char*)"P"   , 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"600E.84P_DL", 600, cardLineMode::CLIENTMODE_LXTF20E, 836841825, 7, (char*)"P_DL", 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"600E.84P_ML", 600, cardLineMode::CLIENTMODE_LXTF20E, 836841825, 7, (char*)"P_ML", 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"500E.84P"   , 500, cardLineMode::CLIENTMODE_LXTF20E, 836841825, 7, (char*)"P"   , 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"500E.84P_DL", 500, cardLineMode::CLIENTMODE_LXTF20E, 836841825, 7, (char*)"P_DL", 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"500E.84P_ML", 500, cardLineMode::CLIENTMODE_LXTF20E, 836841825, 7, (char*)"P_ML", 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"400E.84P"   , 400, cardLineMode::CLIENTMODE_LXTF20E, 836841825, 7, (char*)"P"   , 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"400E.84P_DL", 400, cardLineMode::CLIENTMODE_LXTF20E, 836841825, 7, (char*)"P_DL", 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"400E.84P_ML", 400, cardLineMode::CLIENTMODE_LXTF20E, 836841825, 7, (char*)"P_ML", 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"600E.84S"   , 600, cardLineMode::CLIENTMODE_LXTF20E, 836841825, 7, (char*)"S"   , 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"600E.84S_DL", 600, cardLineMode::CLIENTMODE_LXTF20E, 836841825, 7, (char*)"S_DL", 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"600E.84S_ML", 600, cardLineMode::CLIENTMODE_LXTF20E, 836841825, 7, (char*)"S_ML", 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"500E.84S"   , 500, cardLineMode::CLIENTMODE_LXTF20E, 836841825, 7, (char*)"S"   , 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"500E.84S_DL", 500, cardLineMode::CLIENTMODE_LXTF20E, 836841825, 7, (char*)"S_DL", 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"500E.84S_ML", 500, cardLineMode::CLIENTMODE_LXTF20E, 836841825, 7, (char*)"S_ML", 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"400E.84S"   , 400, cardLineMode::CLIENTMODE_LXTF20E, 836841825, 7, (char*)"S"   , 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"400E.84S_DL", 400, cardLineMode::CLIENTMODE_LXTF20E, 836841825, 7, (char*)"S_DL", 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"400E.84S_ML", 400, cardLineMode::CLIENTMODE_LXTF20E, 836841825, 7, (char*)"S_ML", 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"300E.63P"   , 300, cardLineMode::CLIENTMODE_LXTF20E, 627631369, 7, (char*)"P"   , 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"300E.63P_DL", 300, cardLineMode::CLIENTMODE_LXTF20E, 627631369, 7, (char*)"P_DL", 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"300E.63P_ML", 300, cardLineMode::CLIENTMODE_LXTF20E, 627631369, 7, (char*)"P_ML", 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"350E.63P"   , 350, cardLineMode::CLIENTMODE_LXTF20E, 627631369, 7, (char*)"P"   , 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"350E.63P_DL", 350, cardLineMode::CLIENTMODE_LXTF20E, 627631369, 7, (char*)"P_DL", 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"350E.63P_ML", 350, cardLineMode::CLIENTMODE_LXTF20E, 627631369, 7, (char*)"P_ML", 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"400E.63P"   , 400, cardLineMode::CLIENTMODE_LXTF20E, 627631369, 7, (char*)"P"   , 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"400E.63P_DL", 400, cardLineMode::CLIENTMODE_LXTF20E, 627631369, 7, (char*)"P_DL", 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
        SupportedLineMode((char*)"400E.63P_ML", 400, cardLineMode::CLIENTMODE_LXTF20M, 627631369, 7, (char*)"P_ML", 2, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),

        SupportedLineMode((char*)"125M.33P", 125, cardLineMode::CLIENTMODE_LXTF20M, 327581789, 7, (char*)"P", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"125M.33S", 125, cardLineMode::CLIENTMODE_LXTF20M, 327581789, 7, (char*)"S", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"150M.44P", 150, cardLineMode::CLIENTMODE_LXTF20M, 436775718, 7, (char*)"P", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"150M.44S", 150, cardLineMode::CLIENTMODE_LXTF20M, 436775718, 7, (char*)"S", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"175M.33P", 175, cardLineMode::CLIENTMODE_LXTF20M, 327581789, 7, (char*)"P", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"175M.33S", 175, cardLineMode::CLIENTMODE_LXTF20M, 327581789, 7, (char*)"S", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"200M.66P", 200, cardLineMode::CLIENTMODE_LXTF20M, 655163577, 7, (char*)"P", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"200M.66S", 200, cardLineMode::CLIENTMODE_LXTF20M, 655163577, 7, (char*)"S", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"225M.33P", 225, cardLineMode::CLIENTMODE_LXTF20M, 327581789, 7, (char*)"P", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"225M.33S", 225, cardLineMode::CLIENTMODE_LXTF20M, 327581789, 7, (char*)"S", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"250M.44P", 250, cardLineMode::CLIENTMODE_LXTF20M, 436775718, 7, (char*)"P", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"250M.44S", 250, cardLineMode::CLIENTMODE_LXTF20M, 436775718, 7, (char*)"S", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"275M.66P", 275, cardLineMode::CLIENTMODE_LXTF20M, 655163577, 7, (char*)"P", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"275M.66S", 275, cardLineMode::CLIENTMODE_LXTF20M, 655163577, 7, (char*)"S", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"300M.68P", 300, cardLineMode::CLIENTMODE_LXTF20M, 683648950, 7, (char*)"P", 9,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"300M.68S", 300, cardLineMode::CLIENTMODE_LXTF20M, 683648950, 7, (char*)"S", 9,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"325M.87P", 325, cardLineMode::CLIENTMODE_LXTF20M, 873551436, 7, (char*)"P", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"325M.87S", 325, cardLineMode::CLIENTMODE_LXTF20M, 873551436, 7, (char*)"S", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"350M.44P", 350, cardLineMode::CLIENTMODE_LXTF20M, 436775718, 7, (char*)"P", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"350M.44S", 350, cardLineMode::CLIENTMODE_LXTF20M, 436775718, 7, (char*)"S", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"375M.46P", 375, cardLineMode::CLIENTMODE_LXTF20M, 462468407, 7, (char*)"P", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"375M.46S", 375, cardLineMode::CLIENTMODE_LXTF20M, 462468407, 7, (char*)"S", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"400M.52P", 400, cardLineMode::CLIENTMODE_LXTF20M, 524130862, 7, (char*)"P", 38, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"400M.52S", 400, cardLineMode::CLIENTMODE_LXTF20M, 524130862, 7, (char*)"S", 38, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"425M.66P", 425, cardLineMode::CLIENTMODE_LXTF20M, 655163577, 7, (char*)"P", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"425M.66S", 425, cardLineMode::CLIENTMODE_LXTF20M, 655163577, 7, (char*)"S", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"450M.69P", 450, cardLineMode::CLIENTMODE_LXTF20M, 693702610, 7, (char*)"P", 55, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"450M.69S", 450, cardLineMode::CLIENTMODE_LXTF20M, 693702610, 7, (char*)"S", 55, cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"475M.87P", 475, cardLineMode::CLIENTMODE_LXTF20M, 873551436, 7, (char*)"P", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"475M.87S", 475, cardLineMode::CLIENTMODE_LXTF20M, 873551436, 7, (char*)"S", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"500M.95P", 500, cardLineMode::CLIENTMODE_LXTF20M, 952965203, 7, (char*)"P", 6,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"500M.95S", 500, cardLineMode::CLIENTMODE_LXTF20M, 952965203, 7, (char*)"S", 6,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"525M.66P", 525, cardLineMode::CLIENTMODE_LXTF20M, 655163577, 7, (char*)"P", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"525M.66S", 525, cardLineMode::CLIENTMODE_LXTF20M, 655163577, 7, (char*)"S", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"550M.75P", 550, cardLineMode::CLIENTMODE_LXTF20M, 748758374, 7, (char*)"P", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"550M.75S", 550, cardLineMode::CLIENTMODE_LXTF20M, 748758374, 7, (char*)"S", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"575M.87P", 575, cardLineMode::CLIENTMODE_LXTF20M, 873551436, 7, (char*)"P", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"575M.87S", 575, cardLineMode::CLIENTMODE_LXTF20M, 873551436, 7, (char*)"S", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"600M.92P", 600, cardLineMode::CLIENTMODE_LXTF20M, 924936814, 7, (char*)"P", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"600M.92S", 600, cardLineMode::CLIENTMODE_LXTF20M, 924936814, 7, (char*)"S", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"625M.87P", 625, cardLineMode::CLIENTMODE_LXTF20M, 873551436, 7, (char*)"P", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"625M.87S", 625, cardLineMode::CLIENTMODE_LXTF20M, 873551436, 7, (char*)"S", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"650M.87P", 650, cardLineMode::CLIENTMODE_LXTF20M, 873551436, 7, (char*)"P", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"650M.87S", 650, cardLineMode::CLIENTMODE_LXTF20M, 873551436, 7, (char*)"S", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"675M.87P", 675, cardLineMode::CLIENTMODE_LXTF20M, 873551436, 7, (char*)"P", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"675M.87S", 675, cardLineMode::CLIENTMODE_LXTF20M, 873551436, 7, (char*)"S", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"700M.95P", 700, cardLineMode::CLIENTMODE_LXTF20M, 952965203, 7, (char*)"P", 6,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"700M.95S", 700, cardLineMode::CLIENTMODE_LXTF20M, 952965203, 7, (char*)"S", 6,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"725M.87P", 725, cardLineMode::CLIENTMODE_LXTF20M, 873551436, 7, (char*)"P", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"725M.87S", 725, cardLineMode::CLIENTMODE_LXTF20M, 873551436, 7, (char*)"S", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"775M.87P", 775, cardLineMode::CLIENTMODE_LXTF20M, 873551436, 7, (char*)"P", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"775M.87S", 775, cardLineMode::CLIENTMODE_LXTF20M, 873551436, 7, (char*)"S", 5,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"800M.95P", 800, cardLineMode::CLIENTMODE_LXTF20M, 952965203, 7, (char*)"P", 6,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240), 
        SupportedLineMode((char*)"800M.95S", 800, cardLineMode::CLIENTMODE_LXTF20M, 952965203, 7, (char*)"S", 6,  cardLineMode::CARRIERMODESTATUS_SUPPORTED, 240),
};

struct SupportedAdvParms
{
    std::string name;
    std::string dateType;
    std::string value;
    dco_card::DcoCard_Capabilities_SupportedAdvancedParameter_ApDirection direction;
    int         multiplicity;
    dco_card::DcoCard_Capabilities_SupportedAdvancedParameter_ApConfigurationImpact cfgImpact;
    dco_card::DcoCard_Capabilities_SupportedAdvancedParameter_ApServiceImpact servImpact;
    std::string description;

    SupportedAdvParms(char* nm, char* dt, char* vl, int dir, int mp, int ci, int si, char* desc)
      : name(nm)
      , dateType(dt)
      , value(vl)
      , direction(static_cast<dco_card::DcoCard_Capabilities_SupportedAdvancedParameter_ApDirection>(dir))
      , multiplicity(mp)
      , cfgImpact(static_cast<dco_card::DcoCard_Capabilities_SupportedAdvancedParameter_ApConfigurationImpact>(ci))
      , servImpact(static_cast<dco_card::DcoCard_Capabilities_SupportedAdvancedParameter_ApServiceImpact>(si))
      , description(desc)
    {}
};

const std::vector<SupportedAdvParms> supportAdvParmsVal =
{
        SupportedAdvParms((char*)"SCAvg",(char*)"integer", (char*)"0-2", 2, 1, 2, 1, (char*)"Second stage of averaging for the carrier phase estimate"),
        SupportedAdvParms((char*)"FFCRAvgN", (char*)"integer", (char*)"3,5,7,11,15,23,31,47,63,66,99,136", 2, 8, 2, 2, (char*)"Number of SCs used for averaging of the carrier phase estimate"),
        SupportedAdvParms((char*)"InterWaveGainSharing", (char*)"integer", (char*)"0,1", 1, 3, 5, 1, (char*)"FEC gain sharing across two wavelengths"),
        SupportedAdvParms((char*)"RxCDMode", (char*)"integer", (char*)"0,2,3", 2, 1, 5, 1, (char*)" Change CD mode at the receiver"),
        SupportedAdvParms((char*)"TxCDMode", (char*)"integer", (char*)"0,2,3", 1, 1, 5, 1, (char*)"Change CD mode at the transmitter"),
        SupportedAdvParms((char*)"PolTrackSpeed", (char*)"integer", (char*)"0,1,2,3,4,5", 2, 7, 2, 2, (char*)"Change the SOP tracking capability of a given mode"),
        SupportedAdvParms((char*)"NLC", (char*)"integer", (char*)"0,1,2,3,", 2, 2, 2, 2, (char*)"Turn on or off the fiber nonlinearity noise mitigation circuit"),
        SupportedAdvParms((char*)"PNC", (char*)"integer", (char*)"-31:31", 2, 1, 2, 2, (char*)"Turn on or off the fiber nonlinearity noise mitigation circuit"),
        SupportedAdvParms((char*)"rx_fdeq_gain_delta", (char*)"integer", (char*)"TBD", 2, 1, 2, 2, (char*)"Adjust the per sub-carrier gain offset using the RX FDEQ filter"),
        SupportedAdvParms((char*)"tx_fdeq_gain_delta", (char*)"integer", (char*)"TBD", 1, 1, 2, 2, (char*)"Adjust the per sub-carrier gain offset using the TX FDEQ filter"),
};

}
#endif /* CHM6_DATAPLANE_MS_SRC_SIM_SIMDCOCARDCAPABILITIES_H_ */
