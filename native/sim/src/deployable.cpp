#include "sim/deployable.hpp"

#include <algorithm>

namespace sim {

int containerSlots(DeployKind kind) {
    switch (kind) {
        case DeployKind::WoodenBox: return 12;
        case DeployKind::Furnace: return 6;
        case DeployKind::Campfire: return 4;
        // A cupboard holds the upkeep, which is not in yet, and a bag holds you.
        case DeployKind::ToolCupboard: return 6;
        case DeployKind::SleepingBag: return 0;
    }
    return 0;
}

int Container::add(ItemId id, int count) {
    if (id == ItemId::None || count <= 0) return 0;
    const int stack = itemDef(id).stack;
    for (ItemStack& slot : slots) {
        if (count <= 0) break;
        if (slot.id != id) continue;
        const int put = std::min(stack - slot.count, count);
        slot.count += put;
        count -= put;
    }
    for (ItemStack& slot : slots) {
        if (count <= 0) break;
        if (slot.id != ItemId::None) continue;
        const int put = std::min(stack, count);
        slot = ItemStack{id, put};
        count -= put;
    }
    return count;
}

int Container::count(ItemId id) const {
    int total = 0;
    for (const ItemStack& slot : slots) {
        if (slot.id == id) total += slot.count;
    }
    return total;
}

int Container::take(ItemId id, int count) {
    int taken = 0;
    for (ItemStack& slot : slots) {
        if (count <= 0) break;
        if (slot.id != id) continue;
        const int off = std::min(slot.count, count);
        slot.count -= off;
        count -= off;
        taken += off;
        if (slot.count <= 0) slot = ItemStack{};
    }
    return taken;
}

bool deployableOf(ItemId id, DeployKind& out) {
    switch (id) {
        case ItemId::Campfire: out = DeployKind::Campfire; return true;
        case ItemId::Furnace: out = DeployKind::Furnace; return true;
        case ItemId::ToolCupboard: out = DeployKind::ToolCupboard; return true;
        case ItemId::WoodenBox: out = DeployKind::WoodenBox; return true;
        case ItemId::SleepingBag: out = DeployKind::SleepingBag; return true;
        default: return false;
    }
}

ItemId itemOf(DeployKind kind) {
    switch (kind) {
        case DeployKind::Campfire: return ItemId::Campfire;
        case DeployKind::Furnace: return ItemId::Furnace;
        case DeployKind::ToolCupboard: return ItemId::ToolCupboard;
        case DeployKind::WoodenBox: return ItemId::WoodenBox;
        case DeployKind::SleepingBag: return ItemId::SleepingBag;
    }
    return ItemId::None;
}

}  // namespace sim
