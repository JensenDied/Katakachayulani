#include <Katakachayulani.h>
#include <daemon.h>

inherit "/std/daemon";
inherit "/mod/daemon/control";

/*
descriptor array query_affiliation_subjective_specialties(object who) {
    descriptor array out = ::query_affiliation_subjective_specialties(who);
    object link = who->query_affiliation_link(project_control());
    unless(link)
        return out;
    string array rewards = link->katakachayulani_query_rewards();
    foreach(string reward : rewards) {
        descriptor array specs = Katakachayulani_Reward(reward)->query_reward_specialty_access();
        if(specs)
            out += specs;
    }
    return out;
}
*/
// -- Host composition support functions
descriptor array katakachayulani_find_appropriate_elements(object who) {
    unless(who)
        return 0;
    descriptor array dxrs = ({});
    foreach(descriptor dxr : who->query_elements()) {
        if(Element_Flag_Check(dxr, Element_Flag_Independent) && !Element_Query_Info(dxr, "From_Katakachayulani"))
            continue;
        int material_code = Element_Query(dxr, Element_Type);
        if(member(Katakachayulani_Transmutable_Materials, material_code) != -1)
            dxrs += ({ dxr });
    }
    return dxrs;
}

descriptor array katakachayulani_find_elements(object who) {
    unless(who)
        return 0;
    descriptor array dxrs = ({});
    foreach(descriptor dxr : who->query_elements()) {
        if(Element_Query(dxr, Element_Type) == Material_Kacha && Element_Query_Info(dxr, "From_Katakachayulani"))
            dxrs += ({ dxr });
    }
    return dxrs;
}

descriptor array katakachayulani_find_appropriate_organs(object who) {
    unless(who)
        return 0;
    descriptor array dxrs = ({});
    foreach(descriptor dxr : who->query_organs()) {
        if(Organ_Flag_Check(dxr, Organ_Flag_Independent) && !Organ_Query_Info(dxr, "From_Katakachayulani"))
            continue;
        foreach(int material : Katakachayulani_Transmutable_Materials)
            if(member(Organ_Query(dxr, Organ_Materials), material) != Null)
                dxrs += ({ dxr });
    }
    return dxrs;
}

descriptor array katakachayulani_find_organs(object who) {
    unless(who)
        return 0;
    descriptor array dxrs = ({});
    foreach(descriptor dxr : who->query_organs()) {
        if((member(Organ_Query(dxr, Organ_Materials), Material_Kacha) != Null) && Organ_Query_Info(dxr, "From_Katakachayulani"))
            dxrs += ({ dxr });
    }
    return dxrs;
}

void configure() {
    ::configure();
    set_creator("elronuan");
}
