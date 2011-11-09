#include <Katakachayulani.h>
#include <item.h>
#include <Public.h>
#include <Wild_Talents.h>
#include <services_affiliations.h>

inherit "/std/item";

void perform_psychic_repulse(object who, int power) {
    descriptor dxr = Special_Attack(([
        Special_Attack_Type         : ({ "psionic" }),
        Special_Attack_Damage       : power,
        Special_Attack_Power        : Special_Attack_Power_Powerful,
        Special_Attack_From         : ({
            "out of the", this_object()
        }),
        Special_Attack_Target       : who->find_organ(Organ_Type_Brain)[Organ_Component],
        Special_Attack_Strike       : Special_Attack_Strike_Accurate,
        Special_Attack_Vector       : Vector_Pulse,
        Special_Attack_Size         : power,
        Special_Attack_Pass_Flags   : Attack_Flag_Bypass_Armour | Attack_Flag_Inflict_Mental_Disorder | Attack_Flag_Surprise | Attack_Flag_Inerrant,
        Special_Attack_Flags        : Special_Attack_Flag_Always_Display | Special_Attack_Flag_Limb_Target_Preset
    ]));
    Special_Attack_Execute(dxr, this_object(), who);
}

mixed check_katakachayulani_join(object who, int potency) {
    // No longer an affiliation, having a psionic matrix already bonded should already prevent this from needing to exist
    //if(who->query_affiliation(project_control()))
    //   return "Your Katakachayulani is interfering with your attempt to awaken the dormant spirit in this katakacha";
    if(who->query_affiliation("the Kazarzeth")) {
        object kazarzeth = who->query_affiliation_link("the Kazarzeth");
        if(kazarzeth->query_user() && !kazarzeth->kazarzeth_can_unequip()) {
            perform_psychic_repulse(who, potency);
            return ({
                ({ 's', "lack of control", 0 }), ({ "allows", 0 }), ({ 'r', kazarzeth, 0 }), "to interfer with", ({ 's', 0, "concentration" })
                   });
        }
    }
    if(who->query_affiliation("the Kazarak")) {
        perform_psychic_repulse(who, max(500, potency * 3));
        return "How did you get psionics as a kaz?";
    }
}

status is_psionic_matrix() {
    return True;
}

object set_psionic_matrix_bond(object who, object link) {
    int potency = 0;
    if(extern_call()) {
        object from = previous_object();
        if(load_name(from) == Wild_Talents_Psionics("bond"))
            potency = from->query_potency();
    }
    if(potency <= 300) {
        perform_psychic_repulse(who, potency);
        return 0;
    }
	mixed res = check_katakachayulani_join(who, potency);
	if(res) {
		notify_fail(res);
        return 0;
    }
    if(query_user())
        who->deutilize_item(this_object(), True, True);
    move(Public_Room("trash"));
    object katakachayulani = new(Katakachayulani_Armour("katakachayulani"))->set_psionic_matrix_bond(who, link);
    who->message(([
        Message_Content     : ({
            ([
                Message_Content     : ({
                    0, ({ "stare", 0 }), "intently at", ({ 'r', who, this_object() }), "as an {{starry}unseen force}", ({ "move", 0 }),  ({ 'p', this_object() }), "rapidly towads", ({ 'r', 0, who->locate_limb(Limb_Type_Chest) }),
                    "and reforming as", katakachayulani,
                }),
            ]),
        }),
        Message_Senses      : Message_Sense_Visual | Message_Sense_Always_Perceptible_To_Source,
    ]));
    who->utilize_item(katakachayulani, 0, 0, Usage_Implant, True);
    return katakachayulani;
}

void configure() {
    ::configure();
    set_creator(({ "elronuan" }));
    alter_identity(([
        //Identity_Code           : "katakachayulani",
        Identity_Known_Nouns    : ({ "katakacha" }),
        Identity_Known_Color    : "white",
        Identity_Special_Names  : ({"DORMANT_KATAKACHAYULANI", "NR", "PREVENT_FETCH_GENERATE", "KATAKACHAYULANI", }),
        //Identity_Flags          : Identity_Flag_Suppress_Name_Code,
        Identity_Flags          : Identity_Flag_Suppress_Elements_If_Known,
    ]));
    /*set_identify_skills(([
        Skill_Arcane_Lore                : 80,
        Skill_Mineral_Lore               : 150,
        Skill_History                    : 200,
    ]));*/
    weapon()->set_weapon_type(Weapon_Type_Shard);
    weapon()->set_ablative(False);
    add_description(Description_Type_Generic);
    add_known_description(({
        0, ({ "are", 0}), "a remnant of the katakacha psychic gestalt which once severely injuried the Kazarin."
    }));
    add_known_description(({
        ({ 'r', 0, ({ "form", "condition" }) }), "are signs of the trauma suffered after the Kazarin corrupted key elements of the katakacha psychic gestalt"
    }));
    add_proportion(([
        Element_Type        : Material_Kacha,
        Element_Proportion  : 1.0,
    ]));
}

