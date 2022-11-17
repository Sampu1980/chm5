#ifndef DCO_SECPROC_ADAPTER_PROTO_DEFS_H_
#define DCO_SECPROC_ADAPTER_PROTO_DEFS_H_ 

#include <google/protobuf/message.h>

#include "infinera/dco6secproc/board/v1/dco6_secproc_board_config.pb.h"
#include "infinera/dco6secproc/board/v1/dco6_secproc_board_state.pb.h"
#include "infinera/dco6secproc/board/v1/dco6_secproc_board_fault.pb.h"
#include "infinera/dco6secproc/dataplane/v2/carrier_config.pb.h"
#include "infinera/dco6secproc/ctrlplane/v1/vlan_settings_state.pb.h"
#include "infinera/dco6secproc/ctrlplane/v1/icdp_state.pb.h"
#include "infinera/dco6secproc/ctrlplane/v1/icdp_config.pb.h"
#include "infinera/dco6secproc/ctrlplane/v1/ofec_config.pb.h"

#include "infinera/dco6secproc/keymgmt/v2/keymgmt_config.pb.h"
#include "infinera/dco6secproc/keymgmt/v2/keymgmt_state.pb.h"
#include "infinera/wrappers/v1/infn_datatypes.pb.h"
#include "infinera/chm6/keymgmt/v2/keymgmt_state.pb.h"

using Dco6SecprocBoardConfig = infinera::dco6secproc::board::v1::Dco6SecprocBoardConfig;
using Dco6SecprocBoardState = infinera::dco6secproc::board::v1::Dco6SecprocBoardState;
using Dco6SecprocFault = infinera::dco6secproc::board::v1::Dco6SecprocFault;
using Dco6SecCarrierConfig = infinera::dco6secproc::dataplane::v2::Dco6SecCarrierConfig;
using Dco6SecProcVlanState = infinera::dco6secproc::ctrlplane::v1::VlanSettings;
using Dco6SecProcIcdpState = infinera::dco6secproc::ctrlplane::v1::Dco6SecIcdpState;
using Dco6SecProcIcdpConfig = infinera::dco6secproc::ctrlplane::v1::Dco6SecIcdpConfig;
using Dco6SecProcOfecConfig = infinera::dco6secproc::ctrlplane::v1::Dco6SecOfecConfig;
using Dco6SecProcKeyMgmtConfig = infinera::dco6secproc::keymgmt::v2::KeyMgmtConfig;
using Dco6SecProcKeyMgmtState = infinera::dco6secproc::keymgmt::v2::KeyMgmtState;
#endif
