bool MenuState::online_party_screen(const MenuFrame &f) const {
  return (f.screen==14 && mode_==3) || (f.screen==26 && f.context==2) || (f.screen==216 && f.context==0) ||
         (f.screen==31 && (f.context==party_context || f.context==invite_context));
}
