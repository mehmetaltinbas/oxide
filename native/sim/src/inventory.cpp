#include "sim/inventory.hpp"

#include <algorithm>

namespace sim {

namespace {

/** Into one run of slots: onto the stacks already there, then into the gaps. */
int addInto(ItemStack* slots, int slotCount, ItemId id, int count) {
    const int stack = itemDef(id).stack;
    for (int i = 0; i < slotCount && count > 0; ++i) {
        if (slots[i].id != id) continue;
        const int room = stack - slots[i].count;
        const int put = std::min(room, count);
        slots[i].count += put;
        count -= put;
    }
    for (int i = 0; i < slotCount && count > 0; ++i) {
        if (slots[i].id != ItemId::None) continue;
        const int put = std::min(stack, count);
        slots[i] = ItemStack{id, put};
        count -= put;
    }
    return count;
}

int takeFrom(ItemStack* slots, int slotCount, ItemId id, int count, int& taken) {
    for (int i = 0; i < slotCount && count > 0; ++i) {
        if (slots[i].id != id) continue;
        const int off = std::min(slots[i].count, count);
        slots[i].count -= off;
        count -= off;
        taken += off;
        if (slots[i].count <= 0) slots[i] = ItemStack{};
    }
    return count;
}

}  // namespace

Inventory::Inventory() {
    // You wake up on the beach with a rock, as in the other game.
    hotbar_[0] = ItemStack{ItemId::Rock, 1};
}

void Inventory::selectSlot(int slot) {
    if (slot < 0 || slot >= kHotbarSlots) return;
    active_ = slot;
}

int Inventory::add(ItemId id, int count) {
    if (id == ItemId::None || count <= 0) return 0;
    // The belt takes what you hold; everything else, and any overflow, goes
    // into the pack behind it.
    if (isBeltItem(id)) count = addInto(hotbar_.data(), kHotbarSlots, id, count);
    count = addInto(pack_.data(), kPackSlots, id, count);
    return count;
}

int Inventory::count(ItemId id) const {
    int total = 0;
    for (const ItemStack& s : hotbar_) {
        if (s.id == id) total += s.count;
    }
    for (const ItemStack& s : pack_) {
        if (s.id == id) total += s.count;
    }
    return total;
}

int Inventory::take(ItemId id, int count) {
    int taken = 0;
    count = takeFrom(hotbar_.data(), kHotbarSlots, id, count, taken);
    takeFrom(pack_.data(), kPackSlots, id, count, taken);
    return taken;
}

}  // namespace sim
