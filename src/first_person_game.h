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
namespace first_person {
inline std::vector<item_location> inventory_locations, cell_locations;
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
    inventory_locations.clear(); inventory_labels.clear();
    for(const auto &loc:you.all_items_loc()) {
        if(loc && !loc->is_null()) { inventory_locations.push_back(loc); inventory_labels.push_back(loc->tname()); }
    }
    cell_locations.clear(); cell_labels.clear();
    if(!has_selected_target) {
        const auto p=you.pos_bub(here); selected_target={here.name(p),p.x(),p.y(),p.z(),false};
        has_selected_target=true;
    }
    const auto p=cell_position();
    cell_reachable=can_reach(you,here,p) && here.accessible_items(p);
    if(here.inbounds(p) && (p==you.pos_bub(here) || you.sees(here,p)) && here.sees_some_items(p,you)) {
        for(item &it:here.i_at(p)) {
            cell_locations.emplace_back(map_cursor(p),&it); cell_labels.push_back(it.tname());
        }
    }
}
}
#endif
