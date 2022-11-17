#ifndef DP_PROTO_DEFS_H_
#define DP_PROTO_DEFS_H_

#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>
#include <json/json.h>

#include <infinera/chm6/common/v2/board_init_state.pb.h>
#include <infinera/chm6/common/v2/dco_card.pb.h>
#include <infinera/chm6/common/v2/dco_keymgmt.pb.h>
#include <infinera/chm6/common/v2/sys_constants.pb.h>
#include <infinera/chm6/common/v2/peer_disc_vlan.pb.h>
#include <infinera/chm6/common/v2/base_program_state.pb.h>

#include <infinera/hal/common/v2/common.pb.h>
#include <infinera/hal/pcp/v2/pcp_defines.pb.h>
#include <infinera/hal/pcp/v2/pcp_config.pb.h>
#include <infinera/hal/pcp/v2/pcp_state.pb.h>
#include <infinera/hal/dataplane/v2/client_base.pb.h>

#include <infinera/chm6/tom/v3/tom_state.pb.h>
#include <infinera/chm6/pcp/v5/pcp_config.pb.h>
#include <infinera/chm6/pcp/v5/pcp_state.pb.h>
#include <infinera/chm6/pcp/v5/pcp_fault.pb.h>

#include <infinera/chm6/dataplane/v3/icdp_state.pb.h>
#include <infinera/hal/dataplane/v2/icdp_state.pb.h>

#include <infinera/wrappers/v1/infn_datatypes.pb.h>

#include <infinera/chm6/dataplane/v3/gige_client_config.pb.h>
#include <infinera/chm6/dataplane/v3/gige_client_state.pb.h>
#include <infinera/chm6/dataplane/v3/gige_client_fault.pb.h>
#include <infinera/chm6/dataplane/v3/gige_client_pm.pb.h>
#include <infinera/chm6/dataplane/v3/lldp_state.pb.h>

#include <infinera/chm6/dataplane/v3/carrier_config.pb.h>
#include <infinera/chm6/dataplane/v3/carrier_state.pb.h>
#include <infinera/chm6/dataplane/v3/carrier_fault.pb.h>
#include <infinera/chm6/dataplane/v3/carrier_pm.pb.h>

#include <infinera/chm6/dataplane/v3/scg_config.pb.h>
#include <infinera/chm6/dataplane/v3/scg_state.pb.h>
#include <infinera/chm6/dataplane/v3/scg_fault.pb.h>
#include <infinera/chm6/dataplane/v3/scg_pm.pb.h>

#include <infinera/chm6/dataplane/v3/otu_config.pb.h>
#include <infinera/chm6/dataplane/v3/otu_state.pb.h>
#include <infinera/chm6/dataplane/v3/otu_fault.pb.h>
#include <infinera/chm6/dataplane/v3/otu_pm.pb.h>

#include <infinera/chm6/dataplane/v3/odu_config.pb.h>
#include <infinera/chm6/dataplane/v3/odu_state.pb.h>
#include <infinera/chm6/dataplane/v3/odu_fault.pb.h>
#include <infinera/chm6/dataplane/v3/odu_pm.pb.h>
#include <infinera/hal/dataplane/v2/odu_config.pb.h>

#include <infinera/chm6/dataplane/v3/xcon_config.pb.h>
#include <infinera/chm6/dataplane/v3/xcon_state.pb.h>

#include <infinera/chm6/dataplane/v3/gcc_pm.pb.h>

#include <infinera/chm6/dataplane/v3/dco_capabilities.pb.h>

#include <infinera/chm6/common/v2/tom_presence_map.pb.h>
#include <infinera/lccmn/tom/v1/tom_presence_map.pb.h>
#include <infinera/lccmn/dataplane/v1/tom_serdes.pb.h>
#include <infinera/lccmn/tom/v1/tom_state.pb.h>

#include <infinera/hal/keymgmt/v2/keymgmt_config.pb.h>
#include <infinera/hal/keymgmt/v2/keymgmt_state.pb.h>
#include <infinera/hal/keymgmt/v2/keymgmt_defines.pb.h>
#include <infinera/chm6/keymgmt/v2/keymgmt_config.pb.h>
#include <infinera/chm6/keymgmt/v2/keymgmt_state.pb.h>

#include <infinera/chm6/dataplane/v3/sync_config.pb.h>
#include <infinera/chm6/dataplane/v3/sync_state.pb.h>

namespace chm6_common    = infinera::chm6::common::v2;
namespace chm6_dataplane = infinera::chm6::dataplane::v3;
namespace lccmn_dataplane = infinera::lccmn::dataplane::v1;

namespace hal_common     = infinera::hal::common::v2;
namespace hal_dataplane  = infinera::hal::dataplane::v2;
namespace wrapper        = infinera::wrappers::v1;
namespace hal_chm6       = infinera::hal::chm6::v2;

namespace lccmn_tom      = infinera::lccmn::tom::v1;
namespace chm6_pcp       = infinera::chm6::pcp::v5;

namespace hal_keymgmt    = infinera::hal::keymgmt::v2;

using VlanConfig = chm6_pcp::VlanConfig;
using VlanState  = chm6_pcp::VlanState;
using Chm6PeerDiscVlanState = infinera::chm6::common::v2::Chm6PeerDiscVlanState;
using PcpAppType = infinera::hal::pcp::v2::AppType;

using IcdpNodeInfo = infinera::hal::common::v2::IcdpNodeInfo;
using IcdpState = infinera::hal::dataplane::v2::IcdpState;
using Chm6IcdpState = infinera::chm6::dataplane::v3::Chm6IcdpState;
using BoolWrapper  = infinera::wrappers::v1::Bool;

using Chm6KeyMgmtConfig = infinera::chm6::keymgmt::v2::KeyMgmtConfig;
using Chm6KeyMgmtState = infinera::chm6::keymgmt::v2::KeyMgmtState;

#endif /* DP_PROTO_DEFS_H_ */

