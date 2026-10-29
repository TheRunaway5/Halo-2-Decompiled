#include "halo2/source_execution_campaign_screen.hpp"

namespace halo2 {
bool source_execution_campaign_screen_call(const SourceExecutionCall &call,
    const SourceExecutionCampaignScreenTarget &target) {
    using Kind=SourceExecutionCallKind;
    if(call.kind!=Kind::StopCampaignScreen && call.kind!=Kind::AcknowledgeCampaignScreenCapture)
        return false;
    if(!call.arguments.empty())throw std::invalid_argument("Trailing arguments for native campaign screen call");
    if(!call.result.empty())throw std::invalid_argument("Unexpected result for void native campaign screen call");
    const auto &execute=call.kind==Kind::StopCampaignScreen?target.stop:target.acknowledge_capture;
    if(execute)execute();
    return true;
}
}
