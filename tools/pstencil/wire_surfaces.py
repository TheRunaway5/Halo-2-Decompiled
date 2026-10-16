#!/usr/bin/env python3
"""Add pStencil dynamic-caster and authored specular seams without replacing shared code."""
from pathlib import Path
import sys
p=Path(sys.argv[1]);s=p.read_text()
if 'pstencil_specular_views' in s:raise SystemExit('already integrated')
def edit(old,new):
 global s
 if s.count(old)!=1:raise ValueError('nonunique renderer anchor: '+old[:110])
 s=s.replace(old,new,1)
edit('    std::unique_ptr<pstencil::Render> pstencil_render;', '    std::unique_ptr<pstencil::Render> pstencil_render;\n    std::vector<std::uint8_t> pstencil_caster_materials;')
edit('        bool emblem_w_depth{};', '        bool emblem_w_depth{};\n        bool pstencil_mask{};')
edit('        source_layer15_passes.resize(materials.size());','''        pstencil_caster_materials.clear();
        for(const auto* material:materials) {
            const auto lod=material->selected_shader_lod;
            pstencil_caster_materials.push_back(lod<material->lods.size() &&
                (material->lods[lod].available_layers&(1u<<10))!=0);
        }
        source_layer15_passes.resize(materials.size());''')
edit('                const auto emblem_w_depth=material.shader_switches[7]==1 && material.forward_vertex==13;', 'xxx') if False else None
# Add the property to both independent pipeline-cache keys and constructors.
anchor='                const auto found=std::find_if(forward_pipelines.begin(),forward_pipelines.end(),'
edit(anchor,'                const bool pstencil_mask=source_material_layers[i]==13;\n'+anchor)
s=s.replace('p.emblem_w_depth==emblem_w_depth;','p.emblem_w_depth==emblem_w_depth && p.pstencil_mask==pstencil_mask;')
s=s.replace('{},emblem_w_depth});','{},emblem_w_depth,pstencil_mask});').replace('controls,emblem_w_depth});','controls,emblem_w_depth,pstencil_mask});')
edit('            depth_state.depthCompareOp=VkCompareOp(p.state.depth_function-0x200);','''            depth_state.depthCompareOp=VkCompareOp(p.state.depth_function-0x200);
            depth_state.stencilTestEnable=p.pstencil_mask;
            depth_state.front=depth_state.back={VK_STENCIL_OP_KEEP,VK_STENCIL_OP_KEEP,VK_STENCIL_OP_KEEP,
                VK_COMPARE_OP_EQUAL,255,0,128};''')
edit('        // Procedural light volumes have an already projected XYZRHW/RGBA11','''        depth_state.stencilTestEnable=VK_FALSE;
        // Procedural light volumes have an already projected XYZRHW/RGBA11''') if '        // Procedural light volumes have an already projected XYZRHW/RGBA11' in s else None
# This unique anchor is immediately after the forward pipeline loop.
edit('        const VkVertexInputBindingDescription volume_binding{','        depth_state.stencilTestEnable=VK_FALSE;\n        const VkVertexInputBindingDescription volume_binding{')
edit('        std::vector<MaterialUniform> light_uniforms;light_uniforms.reserve(effect_light_draw_count+raw_effect_light_draw_count);','''        const bool pstencil_enabled=display.graphics_mode==pstencil::GraphicsMode::pStencil;
        std::size_t pstencil_specular_count{};
        if(pstencil_enabled)for(const auto& view:views)for(const auto& draw:view.raw_effect_lights)
            if(draw.pstencil_specular)pstencil_specular_count+=draw.pstencil_specular->size();
        std::vector<MaterialUniform> light_uniforms;
        light_uniforms.reserve(effect_light_draw_count+raw_effect_light_draw_count+pstencil_specular_count);
        std::vector<std::vector<EffectLightRecord>> pstencil_specular_views(views.size());
        std::vector<std::vector<std::vector<std::size_t>>> pstencil_specular_indices(views.size());
        std::vector<std::vector<Vertex>> pstencil_specular_vertices;
        pstencil_specular_vertices.reserve(pstencil_specular_count);''')
edit('        pstencil::Frame pstencil_frame(views.size());\n        const bool pstencil_enabled=display.graphics_mode==pstencil::GraphicsMode::pStencil;','        pstencil::Frame pstencil_frame(views.size());')
old='''            light_uploads.push_back({textures.at(draw.material_slot).descriptor,
                {reinterpret_cast<const std::byte*>(&uniform),sizeof(uniform)},draw.vertex_bank});
        }
        if(!effect_light_storage && !light_uploads.empty())'''
new='''            light_uploads.push_back({textures.at(draw.material_slot).descriptor,
                {reinterpret_cast<const std::byte*>(&uniform),sizeof(uniform)},draw.vertex_bank});
            auto& specular_indices=pstencil_specular_indices[i].emplace_back();
            if(pstencil_enabled && draw.pstencil_specular)for(const auto& specular:*draw.pstencil_specular) {
                auto& vertices=pstencil_specular_vertices.emplace_back(pstencil::posed_triangles(draw.source,specular.material_slot));
                const auto first=unsigned(vertex_count());append_raw_frame(vertices);
                if(vertex_count()>dynamic_limit)throw FormatError("pStencil specular geometry exceeds frame capacity");
                auto& specular_uniform=light_uniforms.emplace_back(view_material_uniform(views[i],specular.material_slot));
                specular_uniform.program=specular.program.words;
                specular_uniform.vertex_attributes=specular.constants[0];specular_uniform.vertex_constants=specular.constants[1];
                specular_indices.push_back(pstencil_specular_views[i].size());
                pstencil_specular_views[i].push_back({specular.material_slot,first,unsigned(vertices.size()),unsigned(light_uploads.size())});
                light_uploads.push_back({textures.at(specular.material_slot).descriptor,
                    {reinterpret_cast<const std::byte*>(&specular_uniform),sizeof(specular_uniform)},specular.vertex_bank});
            }
        }
        if(!effect_light_storage && !light_uploads.empty())'''
edit(old,new)
edit('                if(pstencil_enabled)pstencil_frame.add(i,draw,raw_light_inputs.size());','                if(pstencil_enabled)pstencil_frame.add(i,draw,raw_light_inputs.size(),pstencil_specular_indices[i][j]);')
edit('        if(pstencil_enabled && !pstencil_frame.empty()) {','''        if(pstencil_enabled)for(std::size_t i=0;i<views.size();++i) {
            for(const auto& object:views[i].objects)pstencil_frame.add_casters(i,object.vertices,pstencil_caster_materials);
            // This explicit resident path owns BSP separately: the remaining
            // ordinary vertices are actual current dynamic/model geometry.
            if(views[i].source_bsp_resident_plan)
                pstencil_frame.add_casters(i,views[i].vertices,pstencil_caster_materials);
        }
        if(pstencil_enabled && !pstencil_frame.empty()) {''')
edit('                            for(const auto draw:light.receivers)effect_light_native->record(command,draw);','''                            for(const auto draw:light.receivers)effect_light_native->record(command,draw);
                            vkCmdSetViewport(command,0,1,&model_view);vkCmdSetScissor(command,0,1,&clip);
                            vkCmdPushConstants(command,pipeline_layout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(model_uniform),&model_uniform);
                            vkCmdBindVertexBuffers(command,0,1,&debug_buffer,&offset);
                            for(const auto receiver:light.specular_receivers) {
                                const auto& draw=pstencil_specular_views[i].at(receiver);material_pipeline(draw.material);
                                const auto descriptor=light_descriptors[draw.binding];constexpr std::uint32_t dynamic_offset=0;
                                vkCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline_layout,0,1,&descriptor,1,&dynamic_offset);
                                vkCmdDraw(command,draw.count,1,draw.first,0);
                            }''')
p.write_text(s)
