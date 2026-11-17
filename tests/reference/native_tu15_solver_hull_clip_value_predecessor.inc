        Environment phase(fp,depth);Memory m(memory,observe);
        const auto reference=m.word(facet+0x14u);
        const auto origin=m.vector(reference);auto difference=m.vector(point);
        asm volatile("subps %1,%0":"+x"(difference):"x"(origin):"memory");
        const auto normal=m.vector(facet);const auto product=multiply(normal,difference);
        const auto projection=sum_three(product);
        push(projection);const auto initial_lower=m.word(lower);
        asm volatile("fcomps %0"::"m"(initial_lower):"memory","st");
        push(projection);std::uint16_t relation;
        asm volatile("fnstsw %0":"=a"(relation));
        std::uint32_t result=1u;
        if((relation&0x4100u)==0) {
            const auto current_upper=m.word(upper);
            asm volatile("fcomps %1;fnstsw %0":"=a"(relation):"m"(current_upper):"memory","st");
            if((std::popcount(unsigned((relation>>8u)&5u))&1u)!=0)
                m.word(0x314836u,upper,projection);
        }else {
            add_scalar(m.word(0x41d7a0u));const auto current_lower=m.word(lower);
            asm volatile("fcoms %1;fnstsw %0":"=a"(relation):"m"(current_lower):"memory");
            if((relation&0x4100u)==0)m.pop_float(0x314849u,lower);
            else asm volatile("fstp %%st(0)":::"memory","st");
            result=2u;
        }
        push(m.word(lower));const auto current_upper=m.word(upper);
        asm volatile("fcomps %1;fnstsw %0":"=a"(relation):"m"(current_upper):"memory","st");
        if(!(relation&0x100u))result=4u;
