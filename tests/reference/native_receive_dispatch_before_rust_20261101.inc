  void datagram(unsigned source,std::span<const std::byte> packet,std::uint32_t epoch) {
    if(PeerSurvivorSelection::is_packet(packet)){
      std::lock_guard lock(mutex);
      if(common_phase()&&selection_supported()&&epoch==survivor_selection->state().context.epoch){
        survivor_selection->receive(source,packet,transfer_time());selection_abort();selection_progress();
      }
      return;
    }
    if(PeerSnapshotCandidate::is_packet(packet)){
      std::shared_ptr<PeerSnapshotCandidate> candidate;
      {std::lock_guard lock(mutex);if(common_phase()&&snapshot_supported())candidate=snapshot_candidate;}
      if(candidate&&epoch==candidate->state().context.epoch)candidate->receive(source,packet,transfer_time());
      return;
    }
    if(PeerCommonCutAgreement::is_packet(packet)){
      // Serialize Abort with the final native publication check. The Rust
      // metadata owner invokes no native/session callbacks; lock ordering is
      // session then common-cut, as for the explicit common actions.
      std::lock_guard lock(mutex);
      if(common_phase()&&common_supported()&&(!epoch_packets()||epoch==common_cut->context().epoch))
        common_cut->receive(source,packet,transfer_time());
      selection_abort();selection_progress();
      return;
    }
    if(PeerHostAgreement::is_packet(packet)){
      std::lock_guard lock(mutex);
      if(peer_handoff_phase()&&agreement_context&&epoch==agreement_context->epoch&&agreement){
        agreement->receive(source,packet,transfer_time());selection_progress();fence_peer_generation();
      }else for(const auto &past:agreement_history)if(epoch==past.context.epoch){past.owner->receive(source,packet,transfer_time());break;}
      return;
    }
    if(PeerMeshQuality::is_packet(packet)){
      std::shared_ptr<PeerMeshQuality> quality;
      {std::lock_guard lock(mutex);if(transport_options.direct_udp)quality=mesh_quality;}
      if(quality&&(!epoch_packets()||epoch==quality->context().epoch))quality->receive(source,packet,transfer_time());
      return;
    }
    if(PeerSourceAdmissions::is_packet(packet)){
      std::shared_ptr<PeerSourceAdmissions> admissions;
      {std::lock_guard lock(mutex);if(transport_options.direct_udp&&native_control_phase())admissions=source_admissions;}
      if(admissions&&(!epoch_packets()||epoch==admissions->context().epoch))admissions->receive(source,packet);
      return;
    }
    if(PeerAuthorityTransfer::is_packet(packet)){
      std::shared_ptr<PeerAuthorityTransfer> transfer;
      {std::lock_guard lock(mutex);if(transport_options.direct_udp&&native_control_phase())transfer=authority;}
      if(transfer&&(!epoch_packets()||epoch==transfer->context().epoch))transfer->receive(source,packet,transfer_time());
      return;
    }
    std::lock_guard lock(mutex);
    fence_peer_generation();
    if(bridge::native_session_datagram(*controller,source,epoch,wire(packet))&&agreement)agreement->observe_host(transfer_time());
    refresh();
  }
