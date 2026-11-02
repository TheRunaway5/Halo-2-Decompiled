  void native_fence(bool force=false,bool selected_failed=false){
    if(!agreement_context)return;
    bridge::NativeSessionFence observation;observation.context=wire_context(*agreement_context);
    const auto learned=agreement?std::optional(agreement->state(transfer_time())):std::nullopt;
    observation.fenced=force||(learned&&learned->fenced);
    observation.decision=wire_decision(learned?learned->decision:std::nullopt);
    observation.selection_started=selection_started;observation.selection_failed=selected_failed;
    apply(bridge::native_session_fence(*controller,observation));
  }
  void preparation_terminal(){native_fence(true);}
  void begin_selection(){
    if(!agreement_context)return;
    bridge::native_session_selection(*controller,wire_context(*agreement_context));refresh();
  }
  std::optional<unsigned> migration_candidate(std::uint64_t now)const{
    if(!projection.peer_handoff_phase)return {};
    bridge::NativeSessionCandidate observed;
    observed.agreement_present=agreement&&agreement_context;
    if(observed.agreement_present){observed.context=wire_context(*agreement_context);observed.agreement=wire_agreement(agreement->state(now));}
    observed.quality_present=bool(mesh_quality);
    if(mesh_quality){observed.quality=wire_context(mesh_quality->context());for(const auto &q:mesh_quality->migration_candidates(now))observed.ranked.push_back(q.peer);}
    observed.selection_supported=selection_supported();
    if(survivor_selection){const auto state=survivor_selection->state();observed.selection_complete=state.selection_complete;observed.selection_failed=state.failed;}
    const auto selected=bridge::native_session_candidate(*controller,observed);
    return selected<16?std::optional(unsigned(selected)):std::nullopt;
  }
