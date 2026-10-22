#pragma once
#ifndef CATA_SRC_FIRST_PERSON_GAME_H
#define CATA_SRC_FIRST_PERSON_GAME_H
#include "first_person.h"
#include "avatar.h"
#include "item.h"
#include "item_location.h"
#include "map.h"
#include "map_selector.h"
#include "mapdata.h"
#include "bodypart.h"
#include "item_pocket.h"
#include "item_contents.h"
#include "translations.h"
#include "creature_tracker.h"
#include "character_id.h"
#include "memory_fast.h"
namespace first_person {
inline std::vector<item_location> inventory_locations, cell_locations, expanded_containers;
inline item_location selected_item;
inline bool expanded(const item_location &loc) {
    return std::find(expanded_containers.begin(),expanded_containers.end(),loc)!=expanded_containers.end();
}
inline void toggle_container(const item_location &loc) {
    if(!loc)return;
    auto i=std::find(expanded_containers.begin(),expanded_containers.end(),loc);
    if(i==expanded_containers.end())expanded_containers.push_back(loc);else expanded_containers.erase(i);
}
inline bool item_accessible(item_location loc) {
    while(loc && loc.has_parent()) {
        const item_pocket *pocket=loc.parent_pocket();
        if(pocket && pocket->sealed())return false;
        loc=loc.parent_item();
    }
    return true;
}
// Rows and locations are built together; pocket headers intentionally have no item location.
inline void append_container(item_location loc,int depth,std::vector<item_location> &locations,
                             std::vector<std::string> &labels,const std::string &suffix="") {
    if(!loc || depth>24)return;
    const bool container=!loc->get_contents().get_all_standard_pockets().empty();
    const bool open=expanded(loc);
    locations.push_back(loc);
    labels.push_back(std::string(depth*2,' ')+(container?(open?"[-] ":"[+] "):"    ")+loc->tname()+suffix);
    if(!open)return;
    int number=0;
    for(item_pocket *pocket:loc->get_contents().get_all_standard_pockets()) {
        ++number;
        const auto capacity=pocket->volume_capacity();
        const auto used=capacity-pocket->remaining_volume();
        std::string name=pocket->get_name().translated();
        if(name.empty())name=std::string(pgettext("fpv","Pocket "))+std::to_string(number);
        name+=" ["+std::to_string(units::to_milliliter(used))+" / "+std::to_string(units::to_milliliter(capacity))+" ml]";
        if(pocket->sealed())name+=" "+std::string(pgettext("fpv","SEALED"));
        locations.emplace_back();labels.push_back(std::string((depth+1)*2,' ')+name);
        if(pocket->sealed() && !pocket->transparent())continue;
        for(item *child:pocket->all_items_top())append_container(item_location(loc,child),depth+2,locations,labels);
    }
}
inline std::vector<std::pair<weak_ptr_fast<monster>,std::uint64_t>> monster_identities;
inline std::uint64_t next_monster_identity=1;
// The two states of a door are one physical target, not separate repeat-action slots.
template<class ID> inline std::string object_state_family(ID initial) {
    std::vector<ID> states{initial};std::string key=initial.str();
    for(size_t i=0;i<states.size() && i<16;++i) {
        const auto &object=states[i].obj();
        for(const auto &state:{object.open,object.close}) {
            if(state.is_null() || !state.is_valid())continue;
            key=std::min(key,state.str());
            if(std::find(states.begin(),states.end(),state)==states.end())states.push_back(state);
        }
    }
    return key;
}
inline std::string target_key(const target &t) {
    map &here=get_map();const tripoint_bub_ms p(t.x,t.y,t.z);
    if(t.creature) {
        const Creature *c=get_creature_tracker().creature_at(p,true);
        if(const Character *ch=dynamic_cast<const Character *>(c))return "npc:"+std::to_string(ch->getID().get_value());
        const auto mon=get_creature_tracker().find(here.get_abs(p));
        if(!mon)return "";
        monster_identities.erase(std::remove_if(monster_identities.begin(),monster_identities.end(),
            [](const auto &entry){return entry.first.expired();}),monster_identities.end());
        for(const auto &entry:monster_identities)if(entry.first.lock()==mon)return "monster:"+std::to_string(entry.second);
        const auto id=next_monster_identity++;monster_identities.emplace_back(mon,id);
        return "monster:"+std::to_string(id);
    }
    const auto abs=here.get_abs(p);
    return "tile:"+std::to_string(abs.x())+":"+std::to_string(abs.y())+":"+std::to_string(abs.z())+":"+
        object_state_family(here.ter(p).id())+":"+object_state_family(here.furn(p).id());
}
inline std::string action_label(const std::string &action) {
    if(action=="attack")return pgettext("fpv","Attack");
    if(action=="pickup")return pgettext("fpv","Pick up");
    if(action=="open")return pgettext("fpv","Open");
    if(action=="close")return pgettext("fpv","Shut");
    if(action=="smash")return pgettext("fpv","Smash");
    return pgettext("fpv","Interact");
}
inline tripoint_bub_ms cell_position() {
    return tripoint_bub_ms(selected_target.x,selected_target.y,selected_target.z);
}
inline bool can_reach(avatar &you,map &here,const tripoint_bub_ms &p) {
    const auto origin=you.pos_bub(here);
    if(!here.inbounds(p) || p.z()!=origin.z() || rl_dist(origin,p)>1) { return false; }
    if(p==origin) { return true; }
    // A diagonal reach cannot cut through the corner between two solid walls.
    if(p.x()!=origin.x() && p.y()!=origin.y() &&
       here.impassable(tripoint_bub_ms(p.x(),origin.y(),p.z())) &&
       here.impassable(tripoint_bub_ms(origin.x(),p.y(),p.z()))) { return false; }
    return you.sees(here,p);
}
inline void refresh_inventory() {
    if(!inventory_open) { return; }
    avatar &you=get_avatar(); map &here=get_map();
    expanded_containers.erase(std::remove_if(expanded_containers.begin(),expanded_containers.end(),
        [](const item_location &loc){return !loc;}),expanded_containers.end());
    inventory_locations.clear(); inventory_labels.clear();
    for(const auto &loc:you.all_items_loc()) {
        if(loc && !loc->is_null() && !loc.has_parent()) {
            const std::string state=you.is_worn(*loc)?pgettext("fpv"," [worn]"):pgettext("fpv"," [wielded]");
            append_container(loc,0,inventory_locations,inventory_labels,state);
        }
    }
    body_statuses.clear();
    for(const auto &bp:you.get_all_body_parts(get_body_part_flags::only_main)) {
        body_status status{body_part_name(bp),you.get_part_hp_cur(bp),you.get_part_hp_max(bp),{}};
        status.key=bp.id().str();
        for(const auto &loc:you.top_items_loc())if(loc && loc->covers(bp))status.equipment.push_back(loc->tname());
        body_statuses.push_back(std::move(status));
    }
    morale_level=you.get_morale_level();morale_entries=you.get_morale_entries();
    item_details=selected_item?selected_item->tname()+" | "+selected_item.describe(&you):"";
    cell_locations.clear(); cell_labels.clear();
    if(!has_selected_target) {
        const auto p=you.pos_bub(here); selected_target={here.name(p),p.x(),p.y(),p.z(),false};
        has_selected_target=true;
    }
    const auto p=cell_position();
    cell_reachable=can_reach(you,here,p) && here.accessible_items(p);
    if(here.inbounds(p) && (p==you.pos_bub(here) || you.sees(here,p)) && here.sees_some_items(p,you)) {
        for(item &it:here.i_at(p)) {
            append_container(item_location(map_cursor(p),&it),0,cell_locations,cell_labels);
        }
    }
    // Follow the selected actual item as rows expand/collapse, never silently select another item.
    const auto &locations=selected_side==0?inventory_locations:cell_locations;
    const auto selected=std::find(locations.begin(),locations.end(),selected_item);
    selected_row=selected_item && selected!=locations.end()?std::distance(locations.begin(),selected):-1;
}
}
#endif
