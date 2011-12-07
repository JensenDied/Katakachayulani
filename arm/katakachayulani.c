#include <Empathic_Bonds.h>
#include <Katakachayulani.h>
#include <Public.h>
#include <compare.h>
#include <config.h>
#include <effects.h>
#include <interval.h>
#include <item.h>
#include <learning.h>
#include <learning_source.h>
#include <learning_type.h>
#include <organ.h>
#include <race.h>
#include <sentience.h>
#include <skills.h>
#include <specialty.h>
#include <specialty_access.h>

inherit "/mod/character/energy";
inherit "/std/item";

#define Force_Attribute_Code(what)       \
	unless(intp(what)) what = Attribute(what)->query_attribute_code();
#define Force_Skill_Code(what)           \
	unless(intp(what)) what = Skill(what)->query_skill_code();
#define Check_Skills_Storage             \
    skills ||= ({});                     \
	if(sizeof(skills) < Daemon_Skills->query_skills_array_size()) {\
		skills = skills + allocate(Daemon_Skills->query_skills_array_size() - sizeof(skills));\
	}

private descriptor init_skill(int skill);

internal int high_skill;
internal int will_update_specialty_access;
internal mapping levels_cache;
internal object manager; // WT Link.
internal record shapechange;
internal int array working_attributes;

private descriptor array skills;
//private float real_hit_points;
//private float real_max_hit_points;
private float endurance;
private float max_endurance;
private float max_spell_points;
private float spell_points;
private int capacity;
private mapping specialty_access;
private object owner;
private string bond;
private string owner_extant;
private mapping peak_proportion;
private mapping peak_volume;
private int array attribute_development;
private int array attributes;
private int array permanent_attribute_adjustment;

float katakachayulani_query_proportion();
float katakachayulani_update_peak_proportion();
int query_specialty_bonus(mixed skill);
int query_specialty_required(mixed skill);
status katakachayulani_maintenance();
varargs int query_skill(mixed what, status unmod, float factor, mapping train, int flags);
void set_skill_exp(mixed what, mixed val);
void update_psionic_matrix_skill_information();
varargs void add_skill_exp(mixed what, mixed val, int interval, mixed sources, object instructor);
void set_will_update_specialty_access(status val);
descriptor array query_specialties_provided();
varargs void update_specialty_access(status op, status silent);
varargs status calculate_vitals(status exec);
varargs int query_effective_skill(mixed what, mixed skilled, status unmod, float factor, mapping train, int flags);

float query_katakachaylani_race_factor() {
    peak_volume ||= ([]);
    katakachayulani_update_peak_proportion();
    float count = to_float(sizeof(peak_proportion));
    float value = accumulate(values(peak_proportion));
    return value / count + (count - 1) * 0.05;
}

float query_katakachaylani_rating() {
    return katakachayulani_query_proportion() * query_katakachaylani_race_factor();
}

//
// -- Symbiote Stub Object Functions --
//

object race() {
    return Race("katakacha");
}

object permanent_race() {
    return Race("katakacha");
}

object sentience() {
    return Sentience(Sentience_Anthropic);
}

//
// -- Symbiote Attributes --
//

private varargs void attribute_updated(int what, status nocalc, mixed array ranges) {
	ranges ||= permanent_race()->query_race_subjective_attribute_ranges(this_object());
	int prev = working_attributes[what];
	int calc = Attribute_Level(attributes[what], attribute_development[what], ranges[0][what], ranges[1][what]);
	if(permanent_attribute_adjustment)
		calc += permanent_attribute_adjustment[what];
	switch(what) {
	case Attr_Str           :
	case Attr_Con           :
		break;
	}
	if(prev != calc) {
		working_attributes[what] = calc;
		unless(nocalc) {
			switch(what) {
			case Attr_Int   :
				calculate_vitals();
				break;
			case Attr_Dex   :
				//calculate_speed();
				break;
			case Attr_Siz   :
				update_configuration();
				calculate_vitals();
				break;
			case Attr_Con   :
				calculate_vitals();
				break;
			case Attr_Wil   :
				//constrain_effort();
				calculate_vitals();
				break;
			}
			//update_cached_ratings();
		}
	}
}

void update_attributes(status nocalc, mixed array ranges) {
	ranges ||= permanent_race()->query_race_subjective_attribute_ranges(this_object());
	for(int attr = 0; attr < Attributes_Count; attr++)
		attribute_updated(attr, nocalc, ranges);
}

varargs void set_attribute_development(mixed what, int amount, status nocalc) {
	Force_Attribute_Code(what);
	attribute_development[what] = amount;
	unless(nocalc)
		attribute_updated(what);
}

int query_attribute_development(mixed what) {
	Force_Attribute_Code(what);
	return attribute_development[what];
}

varargs void add_attribute_development(mixed what, int amount, status nocalc) {
	Force_Attribute_Code(what);
	set_attribute_development(what, query_attribute_development(what) + amount, nocalc);
}

varargs void set_permanent_attribute_adjustment(mixed what, int amount, status nocalc) {
	Force_Attribute_Code(what);
	if(!permanent_attribute_adjustment)
		permanent_attribute_adjustment = allocate(Attributes_Array_Size);
	permanent_attribute_adjustment[what] = amount;
	if(all(permanent_attribute_adjustment, #'==, 0))
		permanent_attribute_adjustment = 0;
	unless(nocalc)
		attribute_updated(what);
}

int query_permanent_attribute_adjustment(mixed what) {
	Force_Attribute_Code(what);
	return permanent_attribute_adjustment && permanent_attribute_adjustment[what];
}

varargs void add_permanent_attribute_adjustment(mixed what, int amount, status nocalc) {
	Force_Attribute_Code(what);
	set_permanent_attribute_adjustment(what, query_permanent_attribute_adjustment(what) + amount, nocalc);
}

int query_original_attribute(mixed what) {
	Force_Attribute_Code(what);
	return attributes[what];
}

varargs void set_original_attribute(mixed what, int amount, status nocalc) {
	Force_Attribute_Code(what);
	attributes[what] = amount;
	unless(nocalc)
		attribute_updated(what);
}

int array query_original_attributes() {
	return attributes;
}

varargs void set_attribute(mixed what, int amount, status nocalc) {
	Force_Attribute_Code(what);
	attributes[what] = amount - (working_attributes[what] - attributes[what]);
	unless(nocalc)
		attribute_updated(what);
}

varargs int query_attribute(mixed what, status base) {
	Force_Attribute_Code(what);
	int out = working_attributes[what];
	return out < 0 ? 0 : out;
}

int array query_attributes() {
	return working_attributes;
}

varargs float query_attribute_relative(mixed what, status base, string as_race) {
	Force_Attribute_Code(what);
	object def = as_race ? Race(as_race) : race();
	mixed array ranges = def->query_race_subjective_attribute_ranges(this_object());
	int low = ranges[0][what];
	int high = ranges[1][what];
	float avg = (high + low) / 2.0;
	float rng = high - avg;
	float dev = query_attribute(what, base) - avg;
	return dev * 100 / rng;
}

varargs int set_attribute_relative(mixed what, mixed rel, int offset, string as_race) {
	Force_Attribute_Code(what);
	object def = as_race ? Race(as_race) : race();
	mixed array ranges = def->query_race_subjective_attribute_ranges(this_object());
	int low = ranges[0][what];
	int high = ranges[1][what];
	float avg = (high + low) / 2.0;
	float rng = high - avg;
	int val = to_int(avg + rng * rel / 100);
	set_original_attribute(what, val);
	if(offset)
		set_attribute_development(what, offset);
	return val;
}

varargs int query_average_attribute(mixed what, int off, status flag) {
	unless(what)
		what = Basic_Attributes;
	int ret = round(compute_proportionally(#'query_attribute, what, flag));
	return off ? off + ret * (100 - off) / 100 : ret;
}

varargs int query_attribute_bonus(mixed what, int flag) {
	int rating = query_attribute(what, flag);
	if(rating >= 60)
		return (rating - 50) / 10;
	if(rating < 50)
		return rating / 5 - 10;
	return 0;
}

int query_attribute_racial_minimum(mixed what) {
	Force_Attribute_Code(what);
	return race()->query_race_subjective_attribute_ranges(this_object())[0][what];
}

int query_attribute_racial_maximum(mixed what) {
	Force_Attribute_Code(what);
	return race()->query_race_subjective_attribute_ranges(this_object())[1][what];
}

int query_attribute_balance(mixed what) {
	int val = query_attribute(what) - 50;
	if(val < 0)
		return val / 2;
	else
		return val;
}

int query_physique() {
	return query_attribute_bonus(Attr_Str) + query_attribute_bonus(Attr_Con) + query_attribute_bonus(Attr_Dex) + query_attribute(Attr_Siz) / 10;
}

int query_psyche() {
	return query_attribute_bonus(Attr_Int) + query_attribute_bonus(Attr_Wil) + query_attribute_bonus(Attr_Cha) + query_attribute_bonus(Attr_Per);
}

varargs int initialize_attributes(string as_race, status generation) {
	object def = as_race ? Race(as_race) : permanent_race();
	mixed array ranges = def->query_race_subjective_attribute_ranges(this_object());
	int array low = ranges[0];
	int array high = ranges[1];
	if(generation) {
		float out = 0.0;
		foreach(object attr : Daemon_Attributes->query_attributes()) {
			int attr_code = attr->query_attribute_code();
			float add = attr->query_attribute_assignable_points(low[attr_code], high[attr_code]);
			int num = round(add / 2.0);
			set_attribute(attr_code, low[attr_code] + num, True);
			out += add - num;
		}
		update_attributes(True, !as_race && ranges);
		return round(out);
	}
	int array start = allocate(Attributes_Count);
	for(int i = 0; i < Attributes_Count; i++) {
		int dev = random(Stat_Deviation) - random(Stat_Deviation);
		attributes[i] = low[i] + (high[i] - low[i]) * (100 + dev) / 200;
	}
    int array basic = Basic_Attributes;
    int array dev = allocate(Attributes_Count);
    //int assign = (query_level() - 1) * Attribute_Points_Per_Level;
    int assign = 0;
    int total = 0;
    foreach(int attr : basic)
        total += attributes[attr];
    int assigned = 0;
    foreach(int attr : basic) {
        int amt = assign * attributes[attr] / total;
        attribute_development[attr] = amt;
        assigned += amt;
    }
    while(assigned < assign) {
        attribute_development[random_element(basic)]++;
        assigned++;
    }
	update_attributes(True, !as_race && ranges);
	return 0;
}

//
// -- Symbiote Energy --
//
int query_max_spell_points() {
    return round(max_spell_points);
}

float query_real_max_spell_points() {
    return max_spell_points;
}

int query_spell_points() {
    return round(spell_points);
}

float query_real_spell_points() {
    return spell_points;
}

void set_max_spell_points(mixed val) {
    max_spell_points = to_float(val);
    if(max_spell_points < Min_Cur_SP)
        max_spell_points = Min_Cur_SP;
    if(spell_points > max_spell_points)
        spell_points = max_spell_points;
}

void set_spell_points(mixed val) {
    spell_points = to_float(val);
    if(spell_points > max_spell_points)
        spell_points = max_spell_points;
    else if(spell_points < Min_Cur_SP)
        spell_points = Min_Cur_SP;
}

void add_spell_points(mixed val) {
    spell_points += to_float(val);
    if(spell_points > max_spell_points)
        spell_points = max_spell_points;
    else if(spell_points < Min_Cur_SP)
        spell_points = Min_Cur_SP;
    //if(val < 0)
    //    restart_autoheal();
}

status check_spell_points(mixed amt) {
    amt = to_float(amt);
    if(amt > spell_points)
        return notify_fail(({
            ({ 's', 0, "spiritual energy" }), "is too weak",
        }));
    add_spell_points(-amt);
    return True;
}

void set_real_max_hit_points(float val) {
    set_max_durability(val);
}

int query_max_hit_points() {
    return round(query_max_durability());
}

float query_real_max_hit_points() {
    return query_max_durability();
}

void set_real_hit_points(float val) {
    set_durability(val);
}

int query_hit_points() {
    return round(query_durability());
}

float query_real_hit_points() {
    return query_durability();
}

void set_max_endurance(mixed val) {
    max_endurance = to_float(val);
    if(max_endurance < Min_Cur_END)
        max_endurance = Min_Cur_END;
    if(endurance > max_endurance)
        endurance = max_endurance;
}

int query_max_endurance() {
    return round(max_endurance);
}

float query_real_max_endurance() {
    return max_endurance;
}

int query_endurance() {
    return round(endurance);
}

float query_real_endurance() {
    return endurance;
}

int query_endurance_speed_effect() {
    if(endurance <= 0.0)
        return -20;
    int lev = round(endurance * 100 / max_endurance);
    switch(lev) {
    case 0 .. 75          :
        return (80 - lev) / -4;
    case 76 .. 100        :
        return 0;
    default               :
        error("Inconsistent relative endurance level");
    }
}

void set_endurance(mixed val) {
    val = to_float(val);
    if(val != endurance) {
        int prev = query_endurance_speed_effect();
        endurance = val;
        if(endurance > max_endurance)
            endurance = max_endurance;
        else if(endurance < Min_Cur_END)
            endurance = Min_Cur_END;
        //if(prev != query_endurance_speed_effect())
            //calculate_speed();
    }
}

void add_endurance(mixed val) {
    if(val > 0) {
        int prev = query_endurance_speed_effect();
        endurance += to_float(val);
        if(endurance > max_endurance)
            endurance = max_endurance;
        //if(prev != query_endurance_speed_effect())
            //calculate_speed();
    } else if(val < 0) {
        int prev = query_endurance_speed_effect();
        endurance += to_float(val);
        if(endurance < Min_Cur_END)
            endurance = Min_Cur_END;
        //if(prev != query_endurance_speed_effect())
            //calculate_speed();
        //restart_autoheal();
    }
}

status check_endurance(mixed val) {
    val = to_float(val);
    if(endurance < val)
        return notify_fail(({
                    0, ({ "are", 0 }), "too tired",
                    }));
    add_endurance(-val);
    return True;
}


varargs status calculate_vitals(status exec) {
    mixed val;
    //float adj = (100 - query_malnourishment()) / 100.0;
    float adj = 100.0/100.0;
    int bio = query_skill(Skill_Somatesthesia);
    int hrd = query_skill(Skill_Hardiness);
    int con = query_attribute(Attr_Con);
    int wil = query_attribute(Attr_Wil);
    if(hrd > con)
        hrd = con + (hrd - con) / 3;
    mixed sd = query_skill(Skill_Supernal_Durability);
    sd *= 1.0 - (25 - query_attribute(Attr_Siz) + 1) / 40.0;
    // calculate max HP (Disabled due to using material defaults)
    //val = min(max(((bulk * 5 + (con - 20) * 5 + hrd * 4 + wil + bio / 2) * adj + sd * 20) * HP_Tuning_Factor, Min_HP), Max_HP);
    //set_max_natural_hit_points(val);
    // calculate max SP
    val = (wil * 2.0 + query_attribute_balance(Attr_Per) + query_attribute_balance(Attr_Int) * 3.0 + query_skill(Skill_Centering) + query_skill(Skill_Introspection) / 4) * (2.0 - adj) * SP_Tuning_Factor;
    set_max_spell_points(min(max(val, Min_SP), Max_SP));
    // calculate max END
    val = query_effective_skill(Skill_Stamina) * 4 + wil * 2 + query_skill(Skill_Stamina) + bio;
    set_max_endurance(min(max(val, Min_END), Max_END));
    if(endurance > max_endurance)
        endurance = max_endurance;
    else if(endurance < Min_Cur_END)
        endurance = Min_Cur_END;
    //compute_max_carry();
    //calculate_wounds_speed_effect(True);
    //calculate_speed();
    return True;
}



//TODO
void katakachayulani_do_autoheal(object who) {
    //use Energies, conversion ratios, need, availibility to gather energies from the host.
}

//
// -- Material and Organ spread maintence --
//
// -- Control provided -- Sorta useful for joining requisite checking.
// descriptor array katakachayulani_find_appropriate_elements(object who);
// descriptor array katakachayulani_find_elements(object who);
// descriptor array katakachayulani_find_appropriate_organs(object who);
// descriptor array katakachayulani_find_organs(object who);
//

void katakachayulani_save_race_elements(){
    descriptor array specs = project_control()->katakachayulani_find_appropriate_elements(owner);
    unless(sizeof(specs))
        return;
    foreach(descriptor dxr : specs) {
        unless(Element_Query(dxr, Element_Race) == owner->query_race() && !Element_Query_Info(dxr, "System_Suppress_Race_Element"))
            continue;
        descriptor element = deep_copy(dxr);
        Element_Set_Info(element, "System_Suppress_Race_Element", dxr);
        Element_Set_Info(element, "From_Katakachayulani", True);
        Element_Flag_On(element, Element_Flag_Independent);
        owner->transmute_element(dxr, element);
    }
}

void katakachayulani_save_race_organs(){
    descriptor array specs = project_control()->katakachayulani_find_appropriate_organs(owner);
    unless(sizeof(specs))
        return;
    foreach(descriptor dxr : specs) {
        if(Organ_Query_Info(dxr, "System_Suppress_Race_Organ"))
            continue;
        descriptor organ = deep_copy(dxr);
        Organ_Set_Info(organ, "System_Suppress_Race_Organ", dxr);
        Organ_Set_Info(organ, "Organ_Race", owner->query_race());
        Organ_Set_Info(organ, "From_Katakachayulani", True);
        Organ_Flag_On(organ, Organ_Flag_Independent);
        owner->add_organ(organ);
        owner->remove_organ(dxr);
    }
}


float katakachayulani_query_mass() {
    float mass = 0.0;
    foreach(descriptor dxr : project_control()->katakachayulani_find_elements(owner))
        mass += Element_Query(dxr, Element_Mass);
    foreach(descriptor dxr : project_control()->katakachayulani_find_appropriate_elements(owner))
        mass += Element_Query(dxr, Element_Mass);
    return mass;
}

float katakachayulani_query_old_mass() {
    float mass = 0.0;
    foreach(descriptor dxr : project_control()->katakachayulani_find_elements(owner)) {
        descriptor old = Element_Query_Info(dxr, "System_Suppress_Race_Element");
        mass += Element_Query(old, Element_Mass);
    }
    return mass;
}

float katakachayulani_query_volume() {
    float volume = 0.0;
    foreach(descriptor dxr : project_control()->katakachayulani_find_elements(owner))
        volume += Element_Query(dxr, Element_Volume);
    return volume;
}

float katakachayulani_query_potential_volume() {
    float volume = 0.0;
    foreach(descriptor dxr : project_control()->katakachayulani_find_elements(owner))
        volume += Element_Query(dxr, Element_Volume);
    foreach(descriptor dxr : project_control()->katakachayulani_find_appropriate_elements(owner))
        volume += Element_Query(dxr, Element_Volume);
    return volume;
}

float katakachayulani_query_proportion() {
    float potential_vol = katakachayulani_query_potential_volume();
    return potential_vol > 0.0 ? katakachayulani_query_volume() / potential_vol : 0.0;
}

varargs void katakachayulani_kacha_element_propogation(float restore_vol){
    descriptor array specs = project_control()->katakachayulani_find_appropriate_elements(owner);
    unless(sizeof(specs))
        return False;
    float vol;
    if(restore_vol > Insignificant_Quantity) {
        vol = restore_vol;
    } else {
        vol = owner->query_real_volume()/1500.0 * owner->race()->query_race_elongation();
        float natural_armour = owner->race()->query_natural_armour();
        if(natural_armour > 5.0)
            vol = vol * (5.0 / natural_armour);
    }
    float spec_vol = accumulate(map(specs, (:
            return Element_Query($1, Element_Volume);
        :)));
    vol = min(spec_vol, vol);
    vol = vol / (float)sizeof(specs);
    foreach(descriptor dxr : specs) {
        float conv_vol = min(Element_Query(dxr, Element_Volume), vol);
        dxr = copy(dxr);
        //dxr[Element_Volume] = conv_vol;
        descriptor new_kacha = owner->transmute_element(dxr, ([
            Element_Type    : Material_Kacha,
            Element_Volume  : conv_vol,
        ]));
    }
    //Debug_To("elronuan", ({ "vol propogation", vol }));
}

void katakachayulani_kacha_organ_propogation(){
    descriptor array specs = project_control()->katakachayulani_find_appropriate_organs(owner);
    unless(sizeof(specs))
        return False;
    foreach(descriptor dxr : specs) {
        descriptor organ = deep_copy(dxr);
        int array materials = Organ_Query(organ, Organ_Materials);
        array_specify(&materials, Material_Kacha);
        foreach(int material : materials){
            unless(owner->find_element(material))
                materials -= ({ material });
        }
        Organ_Set(organ, Organ_Materials, materials);
        Organ_Set_Info(organ, "From_Katakachayulani", True);
        Organ_Flag_On(organ, Organ_Flag_Independent);
        owner->set_will_update_configuration(True);
        owner->remove_organ(dxr);
        owner->add_organ(organ);
        owner->update_configuration(True);
    }
}

float katakachayulani_update_peak_volume() {
    peak_volume ||= ([]);
    peak_volume[owner->query_race()] ||= 0.0;
    peak_volume[owner->query_race()] = max(peak_volume[owner->query_race()], katakachayulani_query_volume());
}

float katakachayulani_query_peak_volume() {
    katakachayulani_update_peak_volume();
    return peak_volume[owner->query_race()];
}

float katakachayulani_update_peak_proportion() {
    peak_proportion ||= ([]);
    peak_proportion[owner->query_race()] ||= 0.0;
    peak_proportion[owner->query_race()] = max(peak_proportion[owner->query_race()], katakachayulani_query_proportion());
}

float katakachayulani_query_peak_proportion() {
    katakachayulani_update_peak_proportion();
    return peak_proportion[owner->query_race()];
}

void katakachayulani_composition_snapshot() {
    katakachayulani_save_race_elements();
    katakachayulani_save_race_organs();
    katakachayulani_update_peak_volume();
    katakachayulani_update_peak_proportion();
}

void katakachayulani_start_kacha_propogation() {
    if(katakachayulani_query_proportion() > 0.0)
        return;
    katakachayulani_composition_snapshot();
    float prop = katakachayulani_query_peak_proportion();
    float peak_vol = katakachayulani_query_peak_volume();
    float vol = min(prop * katakachayulani_query_potential_volume(), peak_vol);
    owner->set_will_update_configuration(True);
    katakachayulani_kacha_element_propogation(vol);
    katakachayulani_kacha_organ_propogation();
    owner->update_configuration(True);
}

void katakachayulani_remove_kacha_propogation() {
//*//XXX: Affiliation removal (logout?)
  // Shapeshift race change
    owner->set_will_update_configuration(True);
    filter(owner->query_elements(), (: Element_Query_Info($1, "From_Katakachayulani") && owner->remove_element($1) :));
    filter(owner->query_organs(), (: Organ_Query_Info($1, "From_Katakachayulani") && owner->remove_organ($1) :));
    owner->update_configuration(True);
//*/
}

void katakachayulani_continue_kacha_propogation() {
    unless(katakachayulani_query_proportion() > 0.0)
        return katakachayulani_start_kacha_propogation();
    katakachayulani_kacha_element_propogation();
    katakachayulani_kacha_organ_propogation();
    katakachayulani_composition_snapshot();
}
status katakachayulani_can_unequip() {
    if(query_user() && query_user() == owner)
        return owner->query_disincarnating();
    return True;
}
//
// -- Shapeshift / Race / Size change handling --
//

// Changes within body: warpstone, Basin
// Affiliation based mutations: Shapeshifter, ELF->Ygelleth,
//      Shoggoth Bglaz(I'm unsure of how a shoggoth would Bond with a symbiote, independant organs and all should make them inelgible)
//
// Potions of Attribute_Siz: /def/effect_type/attribute_boost,
// Rings of Growth/Shrinking
// Empathic_Bonds_Charm,
// NOT kurd, ugiors altar.
void katakachayulani_at_race_change(mapping args) {
    if(args["who"] != owner || args["race"] == args["previous"])
        return; // Not important?
    //Debug_To(owner, "Shapechange Debug");
    //Debug_To(owner, shapechange);
    unless(Katakachayulani_Shapechange_Query(shapechange, Katakachayulani_Shapechange_When) == time()) {
        log_file(Katakachayulani_Data("race_change_debug.log"), printable(owner) + " had race changed without shapechange notification\n" + stack_trace());
        return; // Slight protection against directly calling set_race, logging in-case of misconfigured source.
    }
    unless(Katakachayulani_Shapechange_Flag_Check(shapechange, Katakachayulani_Shapechange_Flag_Valid_Source)) {
        owner->set_info("Katakachayulani_Left", "race change");
        owner->remove_affiliation(project_control());
        return;
    }
    if(Katakachayulani_Shapechange_Flag_Check(shapechange, Katakachayulani_Shapechange_Flag_Temp_Race)) {
        if(owner->query_race() == owner->query_permanent_race()) {
            //Debug_To(owner, "At_Race_Change: Returned to perm race");
        } else {
            //Debug_To(owner, "At_Race_Change: Changed non-perm races");
        }
    } else {
        if(owner->query_race() != owner->query_permanent_race()) {
            //Debug_To(owner, "At_Race_Change: Left perm race");
        } else {
            //Debug_To(owner, "At_Race_Change: Continued to be perm race");

        }
    }
}

status katakachayulani_is_valid_shapeshift_source(object source) {
    unless(source) {
        log_file(Katakachayulani_Data("shapechange_source.log"), "Stack trace, shapechange_prenotify without source\n" + stack_trace() +"\n");
        return True; // Anything in this log is likely to be marked invalid, true to be safe
    }
    if(source->is_warpstone() || source->is_affiliation_link())
        return True;
    if(Is_Empathic_Bonds_Charm(source) || Is_Effect_Type(source))
        return True;
    if(begins_with(source, LS_Ring("growth")) || begins_with(source, LS_Ring("shrinking")))
        return True;
    return False;
}

void shapechange_prenotify(object who, object source) {
    // Teardown || detach and wait for post.
    unless(who == owner)
        return;
    Katakachayulani_Shapechange_Set(shapechange, Katakachayulani_Shapechange_Source, source);
    Katakachayulani_Shapechange_Set(shapechange, Katakachayulani_Shapechange_When, time());
    if(katakachayulani_is_valid_shapeshift_source(source))
        Katakachayulani_Shapechange_Flag_On(shapechange, Katakachayulani_Shapechange_Flag_Valid_Source);
    unless(owner->query_race() == owner->query_permanent_race())
        Katakachayulani_Shapechange_Flag_On(shapechange, Katakachayulani_Shapechange_Flag_Temp_Race);
    katakachayulani_composition_snapshot();
    katakachayulani_remove_kacha_propogation();
}

void shapechange_postnotify(object who, object source) {
    // armour()->perform_adapt(who);
    Katakachayulani_Shapechange_Set(shapechange, Katakachayulani_Shapechange_Source, 0);
    Katakachayulani_Shapechange_Set(shapechange, Katakachayulani_Shapechange_When, 0);
    Katakachayulani_Shapechange_Flag_Off(shapechange, Katakachayulani_Shapechange_Flags_All);
    katakachayulani_start_kacha_propogation();
}

//
// -- Symbiote Learning --
//

//TODO
void katakachayulani_update_learning_rewards(){
    //object array rewards = Katakachayulani_Daemon("learning")->query_katakachayulani_rewards(this_object());
}

void katakachayulani_at_skill_experience_add(mapping args) {
    unless(args["who"] && args["who"] == owner)
        return;
    float exp = args["amount"];
    int skill = args["skill"];
    string source;
    if(args["sources"]) {
        switch(typeof(args["sources"])) {
        case T_STRING  :
            source = args["sources"];
            break;
        case T_POINTER :
            mixed array spec;
            if(Is_Learning_Source(args["sources"]))
                spec = ({ args["sources"] });
            if(typeof(args["sources"]) == T_POINTER)
                spec = args["sources"];
            foreach(descriptor dxr : spec) {
                unless(Is_Learning_Source(dxr)) {
                   log_file(Katakachayulani_Data("learning_debug.log"), "Expected Learning Source, had:" + printable(dxr));
                    continue;
                }
                unless(Is_Learning_Type(dxr[Learning_Source_Type])) {
                    log_file(Katakachayulani_Data("learning_debug.log"), "Expected Learning Type, had:" + printable(dxr[Learning_Source_Type]));
                    continue;
                }
                switch(typeof(dxr[Learning_Source_Type][Learning_Type_Spec])) {
                case T_STRING:
                    source = dxr[Learning_Source_Type][Learning_Type_Spec];
                    break;
                case T_POINTER:
                    // I have no idea how multiple learning sources would ever really
                    // combine for this. I'm just going to assume there is only one in here.
                    source = dxr[Learning_Source_Type][Learning_Type_Spec][0];
                    break;
                default:
                    log_file(Katakachayulani_Data("learning_debug.log"), "Invalid Learning Type Spec: " + printable(dxr[Learning_Source_Type][Learning_Type_Spec]));
                    break;
                }
            }
            break;
        default:
            log_file(Katakachayulani_Data("learning_debug.log"), "Unhandled source type, sources had: " + printable(args["sources"]));
        }
    }
    if(args["instructor"] || source && member(({ "memory loss", "memories returning" }), source))
        return;
    unless(exp > 0)
        return;
    add_skill_exp(skill, exp);
}

mixed katakachayulani_mod_description(mapping args) {
    return ({
        Description(([
            Description_Type            : Description_Type_Viewer_Condition,
            Description_Content         : ({
                "Foo",
            }),
            Description_Index           : Condition(Condition_Type_Is_Developer_Or_Test_Character),
        ]))
    });
}

int katakachayulani_mod_cause_damage(descriptor attack) {
    //if(Attack_Flag_Check(attack, Attack_Flag_Hypothetical) || Attack_Query(attack, Attack_Type) != Attack_Type_Strike)
    //    return 0;
    mixed weapon = Attack_Query(attack, Attack_Weapon);
    unless(weapon)
        return 0;// Not item or packed limb.
    unless(Attack_Weapon_Is_Limb(weapon) || weapon->is_armour())
        return 0;
    float mass_factor = katakachayulani_query_mass() - katakachayulani_query_old_mass();
    return round(mass_factor);
}

//
// -- General Maintence routines --
//

void katakachayulani_at_update_configuration(mapping args) {
//    if(!katakachayulani_kacha_check())
}

void katakachayulani_thought_monitor(object who, mixed what, int type, status notify_only) {
    unless(who == owner)
        return;
    //Katakachayulani_Daemon("mentiation")->query_menations_by_type(type)->(who, what, notify_only);
    return;
}

status katakachayulani_maintenance() {
    katakachayulani_continue_kacha_propogation();
    katakachayulani_update_learning_rewards();
    foreach(descriptor mod : owner->query_skill_modifiers())
        if(Modifier_Query_Info(mod, "From_Katakachayulani"))
            ;
    return 0;
}

void katakachayulani_reset_info() {
    peak_proportion = ([]);
    peak_volume = ([]);
    owner->set_organs(({}));
    owner->set_elements(({}));
    owner->remove_info("Wild_Talent_Bond_Time");
}

//
// -- Standard Link Functions --
//

void attach_katakachayulani(object who) {
    owner = who;
    owner_extant = owner->require_extant();
    //owner->require_hook(At_Trait_Update, #'katakachayulani_at_trait_update);
    //owner->require_hook(Mod_Absorb_Damage, #'katakachayulani_mod_absorb_damage);
    owner->require_hook(At_Race_Change, #'katakachayulani_at_race_change);
    owner->require_hook(At_Skill_Experience_Add, #'katakachayulani_at_skill_experience_add);
    owner->require_hook(At_Update_Configuration, #'katakachayulani_at_update_configuration);
    owner->require_hook(Do_Autoheal, #'katakachayulani_do_autoheal);
    owner->require_hook(Mod_Cause_Damage, #'katakachayulani_mod_cause_damage);
    owner->require_hook(Mod_Description, #'katakachayulani_mod_description);
    owner->add_thought_monitor(#'katakachayulani_thought_monitor);
    katakachayulani_start_kacha_propogation();
	update_psionic_matrix_skill_information();
    Interval_Require(#'katakachayulani_maintenance, 60);
}

void detach_katakachayulani(object who) {
    unless(who == owner)
        return;
    Interval_Remove(#'katakachayulani_maintenance);
    owner->remove_hook(At_Skill_Experience_Add, #'katakachayulani_at_skill_experience_add);
    owner->remove_hook(At_Race_Change, #'katakachayulani_at_race_change);
    //owner->remove_hook(At_Trait_Update, #'katakachayulani_at_trait_update);
    owner->remove_hook(At_Update_Configuration, #'katakachayulani_at_update_configuration);
    //owner->remove_hook(Mod_Absorb_Damage, #'katakachayulani_mod_absorb_damage);
    owner->remove_hook(Mod_Cause_Damage, #'katakachayulani_mod_cause_damage);
    owner->remove_hook(Mod_Description, #'katakachayulani_mod_description);
    owner->remove_thought_monitor(#'katakachayulani_thought_monitor);
    owner->remove_hook(Do_Autoheal, #'katakachayulani_do_autoheal);
    owner = 0;
    owner_extant = 0;
}

void reset() {
    ::reset();
    unless(owner)
        return;
    calculate_vitals();
    update_specialty_access();
}

// TESTING
void purge() {
    katakachayulani_reset_info();
}

void data_check(status restoring){
    item::data_check(restoring);
    Check_Skills_Storage;
    attribute_development ||= allocate(Attributes_Array_Size);
    working_attributes ||= allocate(Attributes_Array_Size);
    attributes ||= allocate(Attributes_Array_Size);
    shapechange = allocate(Katakachayulani_Shapechange_Size);
    energy::data_check(False);
}

void preinit() {
    ::preinit();
}

status drop() {
    return owner && True;
}

status query_auto_keep(object who) {
    if(who == owner)
        return True;
    return False;
}

void configure() {
    ::configure();
    set_creator("elronuan");
    alter_identity(([
        Identity_Known_Nouns    : ({ "katakachayulani" }),
        Identity_Known_Color    : "white",
        Identity_Special_Names  : ({
            "KATAKACHAYULANI",
            "NR",
            "PREVENT_FETCH_GENERATE",
            "PSIONIC_MATRIX",
        }),
        Identity_Flags          : Identity_Flag_Suppress_Elements_If_Known,
    ]));
    armour()->set_armour_type(Armour_Type_Symbiotic);
    weapon()->set_ablative(False);
    add_description(Description_Type_Generic);
    add_proportion(([
        Element_Type                    : Material_Kacha,
        Element_Proportion              : 1.0
    ]));
    initialize_attributes();
    add_hook(Can_Unequip_Item, #'katakachayulani_can_unequip);
}

string stat(object view) {
    unless(owner)
        return 0;
    string out = "";
    out += "+-------------------------- Katakachayulani Status  --------------------------+\n";
    out += sprintf("Volume:     %0.5f    Prop: %0.5f\n", katakachayulani_query_volume(), katakachayulani_query_proportion());
    descriptor array specialties = query_specialties_provided();
    if(sizeof(specialties)) {
        specialties = sort_array(specialties, (:
            if(Specialty_Query($1, Specialty_Degree) < Specialty_Query($2, Specialty_Degree))
                return True; 
            if(Specialty_Query($1, Specialty_Degree) == Specialty_Query($2, Specialty_Degree))
                if(Skill(Specialty_Query($1, Specialty_Skills)[0])->query_skill_name() > Skill(Specialty_Query($2, Specialty_Skills)[0])->query_skill_name())
                    return True;
            return False;
        :));
        out += "{{light blue} -- Specialities -------------- : degree - -------------------}{{teal}\n";
        int previous;
        foreach(descriptor spec : specialties) {
            int degree = Specialty_Query(spec, Specialty_Degree);
            if(previous && degree != previous)
                out += "    --------------------------- : ----------------------------\n";
            previous = degree;
            out += "    ";
            out += (Skill(Specialty_Query(spec, Specialty_Skills)[0])->query_skill_name() + (" " * 50))[0..27];
            out += ": ";
            out += left_justify(printable(degree), 9);
            out += "}\n";
        }
    }
    out += "Rating:  " + printable(query_katakachaylani_rating()) + "\n";
    out += "{{bright blue} -- Intervals ------------------{{black}}----------------------------------------------}\n";
    mixed array intervals = filter(m_indices(Daemon_Interval->query_intervals()), (:
        to_object($1) == this_object()
    :));
    if(!sizeof(intervals)) {
        out += "{{black}    none}\n";
    } else {
        foreach(closure interval : intervals) {
            string excess = project_file(object_name(this_object())) + "->";
            string interval_str = printable(interval)[(sizeof(excess))..<3];
            out += "{{dark blue}    " + interval_str + ", " + printable(Interval_Left(interval)) + "/" + printable(Interval_Query(interval)) + "}\n";
        }
    }
    return out;
}

//
// -- Psionic Matrix --
//
status is_psionic_matrix() {
	return True;
}

void set_psionic_matrix_manager(object obj) {
	manager = obj;
}

object query_psionic_matrix_manager() {
	return manager;
}

varargs object query_psionic_matrix_user(status remote) {
	if(!bond)
		return 0;
	object obj = find_object(bond);
	if(!obj)
		return 0;
	if(!remote && environment() != obj)
		return 0;
	return obj;
}

void sever_bond() {
	if(manager) {
		manager->sever_psionic_matrix_bond(this_object());
		manager = 0;
        detach_katakachayulani(owner);
	}
	bond = 0;
    move(Public_Room("trash"));
}


void combat_action(object who, object help, object harm, status def) {
	if(def) {
		switch(high_skill) {
		case Skill_Metacreativity       :
			help->add_resistance_modifier(([
				Modifier_Index          : ({
					"fire",
					"heat",
					"cold",
				}),
				Modifier_Amount         : 20 + random(21),
				Modifier_Duration       : 60 + random(120),
				Modifier_Add_Message    : ({
					0, ({ "feel", 0 }), "a subtle protective field of "
					"energy form around", ({ 'o', 0 }),
				}),
				Modifier_Remove_Message : ({
					0, ({ "feel", 0 }), "some protective energy dissipate "
					"from around", ({ 'o', 0 }),
				}),
			]));
			break;
		case Skill_Metasenses           :
			help->add_attribute_modifier(([
				Modifier_Index          : Attr_Per,
				Modifier_Amount         : 20 + random(21),
				Modifier_Duration       : 120 + random(240),
				Modifier_Add_Message    : ({
					({ 's', 0, "senses" }), "suddenly seem keener",
				}),
				Modifier_Remove_Message : ({
					({ 's', 0, "senses" }), "lose some of their enhanced "
					"keenness",
				}),
			]));
			break;
		case Skill_Psychokinesis        :
			help->add_resistance_modifier(([
				Modifier_Index          : ({
					"crushing",
					"force",
					"slashing",
					"stabbing",
				}),
				Modifier_Amount         : 20 + random(21),
				Modifier_Duration       : 60 + random(120),
				Modifier_Add_Message    : ({
					0, ({ "feel", 0 }), "a subtle protective field of energy "
					"form around", ({ 'o', 0 }),
				}),
				Modifier_Remove_Message : ({
					0, ({ "feel", 0 }), "some protective energy dissipate "
					"from around", ({ 'o', 0 }),
				}),
			]));
			break;
		case Skill_Psycholeptesis       :
			help->add_resistance_modifier(([
				Modifier_Index          : ({
					"electrical",
					"photonic",
					"plasma",
					"radiation",
				}),
				Modifier_Amount         : 20 + random(21),
				Modifier_Duration       : 60 + random(120),
				Modifier_Add_Message    : ({
					0, ({ "feel", 0 }), "a subtle protective field of "
					"energy form around", ({ 'o', 0 }),
				}),
				Modifier_Remove_Message : ({
					0, ({ "feel", 0 }), "some protective energy dissipate "
					"from around", ({ 'o', 0 }),
				}),
			]));
			break;
		case Skill_Psychovorism         :
			help->add_attribute_modifier(([
				Modifier_Index          : ({ Attr_Str, Attr_Con, Attr_Dex }),
				Modifier_Amount         : 20 + random(21),
				Modifier_Duration       : 120 + random(240),
				Modifier_Add_Message    : ({
					0, ({ "feel", 0 }), "energy flowing into",
					({ 'r', 0, "body" }), ", making", ({ 'o', 0 }),
					"stronger, tougher, faster",
				}),
				Modifier_Remove_Message : ({
					0, ({ "feel", 0 }), "some of the energy enhancing",
					({ 'r', 0, "body" }), "fade away",
				}),
			]));
			break;
		case Skill_Redaction            :
			help->display(({
				"healing energies flow through", ({ 's', 0, "body" }),
			}));
			help->heal_living(20 + random(21));
			break;
		case Skill_Telepathy            :
			help->add_attribute_modifier(([
				Modifier_Index          : ({ Attr_Int, Attr_Wil }),
				Modifier_Amount         : 20 + random(21),
				Modifier_Duration       : 120 + random(240),
				Modifier_Add_Message    : ({
					({ 's', 0, "mind" }), "feels energized and enhanced",
				}),
				Modifier_Remove_Message : ({
					0, ({ "feel", 0 }), "some enhancements to",
					({ 'r', 0, "mind" }), "slip away",
				}),
			]));
			break;
		case Skill_Translocation        :
			help->add_resistance_modifier(([
				Modifier_Index          : ({
					"magical",
					"necromantic",
					"temporal",
					"sonic",
				}),
				Modifier_Amount         : 20 + random(21),
				Modifier_Duration       : 60 + random(120),
				Modifier_Add_Message    : ({
					0, ({ "feel", 0 }), "a subtle protective field of "
					"energy form around", ({ 'o', 0 }),
				}),
				Modifier_Remove_Message : ({
					0, ({ "feel", 0 }), "some protective energy dissipate "
					"from around", ({ 'o', 0 }),
				}),
			]));
			break;
		default                          :
			help->add_endurance(40 + random(40));
			help->display(({
				0, ({ "feel", 0 }), "energized",
			}));
			break;
		}
	} else {
		int limb = harm->get_limb_hit(help);
		switch(high_skill) {
		case Skill_Metacreativity   :
			harm->message(({
				"a {{fiery}ball of fire} bursts from",
				({ 's', who, this_object() }), "to strike", 0, ({ 'r', 0, ({ limb })}),
			}));
			harm->do_damage(15 + random(40), help, limb, "fire");
			break;
		case Skill_Psychokinesis    :
			harm->message(({
				"a {{shimmering}shimmering bolt of force} bursts from",
				({ 's', who, this_object() }), "to strike", 0, ({ 'r', 0, ({ limb })}),
			}));
			harm->do_damage(15 + random(40), help, limb, "force");
			break;
		case Skill_Psycholeptesis   :
			harm->message(({
				"a {{lightning}crackling arc of electricity} bursts from",
				({ 's', who, this_object() }), "to strike", 0, ({ 'r', 0, ({ limb })}),
			}));
			harm->do_damage(15 + random(40), help, limb, "electrical");
			break;
		case Skill_Translocation    :
			harm->message(({
				"everything around", harm, "seems to {{chaotic}twist and "
				"distort bizarrely} for a moment",
			}));
			harm->stun_living(6 + random(12), False, "chaos");
			break;
		default                     :
			harm->message(({
				"a {{sparkling}lance of half-visible psychic energy} bursts "
				"from", ({ 's', who, this_object() }), "to strike", 0,
			}));
			harm->stun_living(4 + random(14), False, "psionic");
			break;
		}
	}
}

varargs object set_psionic_matrix_bond(object who, object link) {
    manager = link;
    bond = object_name(who);
    if(owner_extant && owner_extant == who->require_extant())
        return this_object();
    attach_katakachayulani(who);
    return this_object();
}


string query_psionic_matrix_bond() {
	return bond;
}

//Psionic Energy.  Functions are wrappers for the psionic matrix interface
mixed query_psionic_energy_conversion_ratio() {
    int conversion_skill = Energy(Energy_Psionic)->query_energy_conversions()[Energy_Spiritual];
    return scale_conversion(Condition_Evaluate(Condition(([
        Condition_Skill_Composite           : True,
        conversion_skill                    : 1.0,
        ([
            Condition_Type_Code             : Condition_Type_Max,
            Condition_Info                  : ({
                ([
                    Condition_Type_Code     : Condition_Type_Attribute,
                    Condition_Info          : Skill(conversion_skill)->query_skill_attribute(),
                    Condition_Value         : 1.0,
                ]),
                ([
                    Condition_Type_Code     : Condition_Type_Attribute,
                    Condition_Info          : Attr_Wil,
                    Condition_Value         : 0.9,
                ]),
            }),
        ])                                  : True,
    ])), this_object(), 0), 0, 100, 20, 100) / 100.0;
}

mixed query_psionic_matrix_capacity() {
    return Energy_Retrieve_Maximum(this_object(), Energy_Psionic);
}

void set_psionic_matrix_energy(mixed val) {
    Energy_Set_Amount(this_object(), Energy_Psionic, val);
}

mixed query_psionic_matrix_energy() {
    return Energy_Retrieve_Amount(this_object(), Energy_Psionic);
}

void add_psionic_matrix_energy(mixed val) {
    int conversion_skill = Energy(Energy_Psionic)->query_energy_conversions()[Energy_Spiritual];
    add_skill_exp(conversion_skill, Learn_Uncommon);
    val /= Energy(Energy_Psionic)->query_energy_potency();
    if(val > 0.0) { // Incoming spirit has a conversion penalty applied
        val *= query_psionic_energy_conversion_ratio();
    }
    Energy_Change_Amount(this_object(), Energy_Psionic, val);
}

void update_psionic_matrix_skill_information() {
	int high_level = Null;
	int level;
	foreach(int skill : Skill_Class(Skill_Class_Psionic)->query_skill_class_contents()->query_skill_code()) {
        level = query_skill(skill);
        if(level > high_level) {
            high_skill = skill;
            high_level = level;
        }
	}
	levels_cache = 0;
}

void set_psionic_matrix_levels(mapping levels) {
	update_psionic_matrix_skill_information();
	levels_cache = 0;
}

mapping query_psionic_matrix_levels() {
	if(levels_cache)
		return levels_cache;
    int array skill_class = Skill_Class(Skill_Class_Psionic)->query_skill_class_contents()->query_skill_code();
    levels_cache = mkmapping(skill_class, map_array(skill_class, #'query_skill));
	return levels_cache;
}

void energy_infusion(int amt) {
    add_psionic_matrix_energy(amt);
}

void check_action() {
    int act, i, j;
    mapping exits, inc;
    string dir, *dirs;
	object who = query_psionic_matrix_user();
	if(who) {
		status esc = False;
		object targ = who->query_attacker();
        act = 30;
		if(targ && random(100) < act && query_psionic_matrix_energy() > 25) {
			status def;
			if(high_skill == Skill_Metasenses || high_skill == Skill_Redaction)
				def = True;
			else if(!random(2))
				def = False;
			else if(!random(2))
				def = True;
			else if(who->query_hp_critical(25) || who->query_mortal_wound())
				def = True;
			else if(targ->query_mortal_wound())
				def = False;
			else
				def = random(2) ? True : False;
			add_psionic_matrix_energy(-25);
			combat_action(who, who, targ, def);
		} else if(targ && random(100) < act && query_psionic_matrix_energy() > 25) {
			status def;
			if(high_skill == Skill_Metasenses || high_skill == Skill_Redaction)
				def = True;
			else if(!random(2))
				def = False;
			else if(!random(2))
				def = True;
			else if(who->query_hp_critical(25) || who->query_mortal_wound())
				def = False;
			else if(targ->query_mortal_wound())
				def = True;
			else
				def = random(2) ? True : False;
			combat_action(who, targ, who, def);
		}
	}
}

void item_restored(object who) {
    attach_katakachayulani(who);
	update_psionic_matrix_skill_information();
	if(bond)
		owner_extant ||= bond->require_extant();
	if(owner_extant)
		all_inventory(who)->restore_psionic_matrix_bond(this_object(), owner_extant);
    calculate_vitals();
}

varargs void remove(int flags) {
	object who = query_psionic_matrix_user(True);
	if(who && !who->query_disincarnating()) {
		if(bond)
			sever_bond();
	}
	::remove(flags);
}

varargs string query_psionic_matrix_report(object who) {
	string out = "";
	mapping map = query_psionic_matrix_levels();
	string array keys = sort_array(m_indices(map), #'>);
	//string name = query_psionic_matrix_name();
	//if(name)
	//	out += "Name: " + name + "\n";
	out += "Psionic activity class modifiers:\n";
	foreach(string key : keys)
		out += "  " + capitalize(Skill(key)->query_skill_name()) + ": " + map[key] + "\n";
	out += "Capacity: " + printable(query_psionic_matrix_capacity()) + "\n";
	out += "Energy: " + printable(query_psionic_matrix_energy()) + "\n";
	out += "Primary capacity: " + Skill(high_skill)->query_skill_name() + "\n";
	return out;
}

//
// -- Skills --
//
int query_specialty(mixed skill) {
    Force_Skill_Code(skill);
    descriptor dxr = skills[skill];
    return dxr ? Learning_Query(dxr, Learning_Specialty) : 0;
}

void set_specialty(mixed what, int val) {
    if(!intp(val) || val < 0)
        check_argument(2, val);
    Force_Skill_Code(what);
    descriptor dxr = skills[what] || init_skill(what);
    int deg = Learning_Query(dxr, Learning_Specialty);
    int bon = deg - Learning_Query(dxr, Learning_Assigned);
    Learning_Set(dxr, Learning_Assigned, val - bon);
    Learning_Set(dxr, Learning_Specialty, val);
    Learning_Specialty_Changed(dxr, this_object(), what, deg);
}

void set_skill_points(mixed what, int val) {
    Force_Skill_Code(what);
    if(val < 0 || val > Max_Skill_Rating)
        error("set_skill_points() for " + Skill(what)->query_skill_name() + " had value outside range 0.." + Max_Skill_Rating);
    descriptor dxr = skills[what] || init_skill(what);
    int max = Skill_Maximum(Learning_Query(dxr, Learning_Specialty), what, this_object());
    if(val > max) {
        int need = Specialty_Needed(val, what);
        int bon = query_specialty_bonus(what);
        set_specialty(what, max(need, bon));
        max = Skill_Maximum(Learning_Query(dxr, Learning_Specialty), what, this_object());
        if(val > max) {
            warn("could not assign specialty allowing autonomon to be assigned skill " + Skill(what)->query_skill_name() + " at " + val);
            return;
        }
    }
    set_skill_exp(what, Rating_to_Experience(val, max));
}

void add_skill_points(mixed what, int val) {
    Force_Skill_Code(what);
    if(skills[what])
        set_skill_points(what, query_skill(what, True) + val);
    else
        set_skill_points(what, val);
}

varargs int query_skill(mixed what, status unmod, float factor, mapping train, int flags) {
    Force_Skill_Code(what);
    descriptor dxr = skills[what];
    int out = dxr && Learning_Query(dxr, Learning_Level);
    if(factor && factor != 1.0)
        out = round(out * factor);
    if(train && (dxr || (flags & Skill_Query_Flag_Always_Train)))
        train[what] += out;
    return out;
}

int array query_skills() {
    int array out = ({});
    for(int ix = sizeof(skills) - 1; ix > 0; ix--)
        if(skills[ix])
            out += ({ ix });
    return out;
}

varargs int query_average_skill(mixed list, status unmod, float factor, mapping train, int off, int flags) {
    float total = compute_proportionally(#'query_skill, list, unmod, factor, train, flags);
    return round(off ? off + total * (100 - off) / 100 : total);
}

varargs int query_effective_skill(mixed what, mixed skilled, status unmod, float factor, mapping train, int flags) {
    return query_skill(what, unmod, factor, train, flags);
}

varargs query_average_effective_skill(mixed list, mixed skilled, status unmod, float factor, mapping train, int off, int flags) {
    float total = compute_proportionally(#'query_effective_skill, list, skilled, unmod, factor, train, flags);
    return round(off ? off + total * (100 - off) / 100 : total);
}

int query_skill_exp(mixed what) {
	Force_Skill_Code(what);
	descriptor dxr = skills[what];
	if(dxr)
		return Learning_Query(dxr, Learning_Experience);
	else
		return 0;
}

int query_skill_maximum(mixed what) {
    Force_Skill_Code(what);
    descriptor dxr = skills[what];
    return Skill_Maximum(dxr && Learning_Query(dxr, Learning_Specialty), what, this_object());
}

private descriptor init_skill(int skill) {
    Check_Skills_Storage;
    object def = Skill(skill);
    descriptor dxr = Learning();
    int req = def->query_skill_specialty_required();
    if(req) {
        descriptor spec = specialty_access[skill];
        unless(spec) {
            spec = Specialty_Access(([
                    Specialty_Access_Degree             : req,
                    Specialty_Access_Required           : req,
                    Specialty_Access_Bonus              : req,
                    Specialty_Access_Recommended        : req,
                ])),
            specialty_access[skill] = spec;
        }
        int prov = Specialty_Access_Query(spec, Specialty_Access_Degree);
        unless(prov >= req)
            return 0;
        int bon = Specialty_Access_Query(spec, Specialty_Access_Bonus);
        int deg = Learning_Query(dxr, Learning_Specialty);
        int assn = Learning_Query(dxr, Learning_Assigned);
        status any = False;
        if(deg - assn != bon) {
            Learning_Set(dxr, Learning_Specialty, assn + bon);
            any = True;
        }
        if(deg < req) {
            Learning_Set(dxr, Learning_Specialty, max(req, bon));
            Learning_Set(dxr, Learning_Assigned, max(req - bon, 0));
            any = True;
        }
        if(any)
            Learning_Specialty_Changed(dxr, this_object(), skill, deg);
    }
    return skills[skill] = dxr;
}

void set_skill_exp(mixed what, mixed val) {
    Force_Skill_Code(what);
    descriptor dxr = skills[what] || init_skill(what);
    Learning_Set(dxr, Learning_Experience, max(to_float(val), 0.0));
    Learning_Experience_Changed(dxr, what, this_object());
}

void remove_skill(mixed what) {
    Force_Skill_Code(what);
    descriptor dxr = skills[what];
    unless(dxr)
        return;
    skills[what] = 0;
}

varargs void add_skill_exp(mixed what, mixed val, int interval, mixed sources, object instructor) {
	if(val == 0)
		return;
	Force_Skill_Code(what);
	descriptor dxr = skills[what] || init_skill(what);
	unless(dxr)
		return;
	if(interval && Learning_Query(dxr, Learning_Trained) + interval > time())
		return;
	int prev = Learning_Query(dxr, Learning_Level);
	if(val < 0 || prev < Skill_Maximum(Learning_Query(dxr, Learning_Specialty), what, this_object())) {
		object def = Skill(what);
		if(Learning_Experience_Gain(dxr, what, this_object(), val, sources)) {
			int curr = Learning_Query(dxr, Learning_Level);
		}
		if(val > 0) {
			int array antags = def->query_skill_antagonisms();
			if(antags) {
				float adj = val / -4.0;
				foreach(int antag : antags) {
					descriptor adxr = skills[antag];
					if(!adxr)
						continue;
					int cur = Learning_Query(adxr, Learning_Experience);
					if(cur > 0)
						add_skill_exp(antag, adj);
				}
			}
		} else {
			if(Learning_Query(dxr, Learning_Experience) <= 0 && !query_specialty_required(what))
				remove_skill(what);
		}
    }
}

descriptor array query_specialties_provided() {
    descriptor array prov = ({});
    for(int ix = sizeof(skills) - 1; ix > 0; ix--) {
        descriptor learn = skills[ix];
        if(!learn)
            continue;
        int lrn_level = Learning_Query(learn, Learning_Level);
        int bon = Specialty_Access_Query(learn, Specialty_Access_Bonus);
        if(lrn_level && lrn_level == query_skill_maximum(ix))
            bon += 1;
        bon = min(bon, Max_Specialty_Degree);
        if(bon) {
            prov += ({
                Specialty(([
                    Specialty_Skills             : ix,
                    Specialty_Degree             : bon,
                    Specialty_Bonus              : bon,
                    Specialty_Recommended        : bon,
                    Specialty_Required           : bon,
                ])),
            });
        }
    }
    if(sizeof(prov))
        return prov;
    return ({});
}

private varargs mapping determine_specialty_access(status silent) {
    Check_Skills_Storage;
    specialty_access = Specialty_Process(query_specialties_provided());
    int array changed = ({});
    foreach(int skill, descriptor spec : specialty_access) {
        descriptor learn = skills[skill];
        int bon = Specialty_Access_Query(spec, Specialty_Access_Bonus);
        if(bon || learn) {
            unless(learn) {
                learn = init_skill(skill);
                array_specify(&changed, skill);
            }
            int deg = Learning_Query(learn, Learning_Specialty);
            int assn = Learning_Query(learn, Learning_Assigned);
            int cur_bon = deg - assn;
            if(cur_bon != bon) {
                Learning_Set(learn, Learning_Specialty, assn + bon);
                Learning_Specialty_Changed(learn, this_object(), skill, deg);
                array_specify(&changed, skill);
            }
        }
        int req = Specialty_Access_Query(spec, Specialty_Access_Required);
        if(req) {
            unless(learn) {
                learn = init_skill(skill);
                array_specify(&changed, skill);
            }
            int deg = Learning_Query(learn, Learning_Specialty);
            if(deg < req) {
                if(req > Specialty_Access_Query(spec, Specialty_Access_Degree))
                    error("Internal inconsistency -- Required " + req + " greater than Degree " + Specialty_Access_Query(spec, Specialty_Access_Degree) + " for skill " + Skill(skill)->query_skill_name());
                int assn = Learning_Query(learn, Learning_Assigned);
                int bon = deg - assn;
                Learning_Set(learn, Learning_Specialty, max(req, bon));
                Learning_Set(learn, Learning_Assigned, max(req - bon, 0));
                Learning_Specialty_Changed(learn, this_object(), skill, deg);
                array_specify(&changed, skill);
            }
        }
    }
    for(int ix = sizeof(skills) - 1; ix > 0; ix--) {
        descriptor learn = skills[ix];
        if(!learn)
            continue;
        int deg = Learning_Query(learn, Learning_Specialty);
        if(!deg)
            continue;
        int orig_deg = deg;
        descriptor spec = specialty_access[ix];
        int bon = deg - Learning_Query(learn, Learning_Assigned);
        int bprov = spec && Specialty_Access_Query(spec, Specialty_Access_Bonus);
        int dprov = spec && Specialty_Access_Query(spec, Specialty_Access_Degree);
        status any = False;
        if(bon > bprov) {
            bon = bprov;
            int asg = Learning_Query(learn, Learning_Assigned);
            deg = asg + bon;
            Learning_Set(learn, Learning_Specialty, deg);
            any = True;
        }
        if(any) {
            Learning_Specialty_Changed(learn, this_object(), ix, orig_deg);
            array_specify(&changed, ix);
        }
    }
    if(sizeof(changed)) {
        mapping degrees = ([]);
        int array removed = ({});
        foreach(int skill : changed) {
            descriptor learn = skills[skill];
            int deg = Learning_Query(learn, Learning_Specialty);
            object def = Skill(skill);
            int req = def->query_skill_specialty_required();
            if(deg < req) {
                remove_skill(skill);
                removed += ({ skill });
                continue;
            }
            degrees[deg] ||= ({});
            degrees[deg] += ({ skill });
        }
        if(sizeof(removed) && !silent) {
            string array names = ({});
            foreach(int skill : removed)
                names += ({ Skill(skill)->query_skill_name() });
            names = sort_array(names, #'compare, COMPARE_CASE_INSENSITIVE);
            display(({
                0, "can no longer specialize in " + list_array(names) + " to "
                "the degree required, and so have lost all use of",
                (sizeof(names) == 1 ? "this ability" : "these abilities"),
            }));
        }
        if(sizeof(degrees) && !silent) {
            int array degrees_sort = sort_array(keys(degrees), #'>);
            foreach(int degree : degrees_sort) {
                string array names = ({});
                foreach(int skill : degrees[degree])
                    names += ({ Skill(skill)->query_skill_name() });
                names = sort_array(names, #'compare, COMPARE_CASE_INSENSITIVE);
                if(degree)
                    display(({
                        0, ({ "are", 0 }), "now",
                        a(numerical(degree, False, True)) + "-degree "
                        "specialist in " + list_array(names),
                    }));
                else
                    display(({
                        0, "no longer specialize in",
                        list_array(names, "or"),
                    }));
            }
        }
    }
    return specialty_access;
}

void set_will_update_specialty_access(status val) {
    if(val)
        will_update_specialty_access++;
    else if(will_update_specialty_access > 0)
        will_update_specialty_access--;
}

mapping query_specialty_access() {
    return specialty_access ||= determine_specialty_access();
}

int query_specialty_available(mixed skill) {
    Force_Skill_Code(skill);
    descriptor dxr = query_specialty_access()[skill];
    return dxr ? Specialty_Access_Query(dxr, Specialty_Access_Degree) : 0;
}

int query_specialty_required(mixed skill) {
    Force_Skill_Code(skill);
    descriptor dxr = query_specialty_access()[skill];
    return dxr ? Specialty_Access_Query(dxr, Specialty_Access_Required) : 0;
}

int query_specialty_bonus(mixed skill) {
    Force_Skill_Code(skill);
    descriptor dxr = query_specialty_access()[skill];
    return dxr ? Specialty_Access_Query(dxr, Specialty_Access_Bonus) : 0;
}

status query_specialty_reveal(mixed what) {
    Force_Skill_Code(what);
    descriptor acc = query_specialty_access()[what];
    unless(acc)
        return False;
    unless(Specialty_Access_Query(acc, Specialty_Access_Flags) & Specialty_Access_Flag_Reveal)
        return False;
    return True;
}

int query_specialty_minimum(mixed skill) {
    Force_Skill_Code(skill);
    descriptor dxr = query_specialty_access()[skill];
    if(dxr)
        return max(Specialty_Access_Query(dxr, Specialty_Access_Required), Specialty_Access_Query(dxr, Specialty_Access_Bonus));
    else
        return 0;
}

int query_will_update_specialty_access() {
    return will_update_specialty_access;
}

varargs void update_specialty_access(status op, status silent) {
    if(will_update_specialty_access)
        if(op == True)
            will_update_specialty_access--;
        else if(op != Null)
            return;
    will_update_specialty_access++;
    specialty_access = determine_specialty_access(silent);
    will_update_specialty_access--;
}

descriptor array query_skill_information() {
    return skills;
}

