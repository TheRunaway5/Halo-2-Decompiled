const SourceBspAdmissionRecord *SourceBspAdmission::instance(std::size_t id) const {
    const auto found=std::find_if(instances.begin(),instances.end(),[&](const auto &r){return r.id==id;});
    return found==instances.end()?nullptr:&*found;
}
const SourceBspAdmissionRecord *SourceObjectAdmission::object(std::uint32_t id) const {
    const auto found=std::find_if(objects.begin(),objects.end(),
        [&](const auto &r){return r.id==id;});
    return found==objects.end()?nullptr:&*found;
}
