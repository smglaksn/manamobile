/*
 * Mana QML plugin
 * Copyright (C) 2012  Erik Schilling 
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "character.h"

#include "protocol.h"
#include "spritelistmodel.h"

#include "resource/itemdb.h"
#include "resource/hairdb.h"
#include "resource/racedb.h"

#include <QDebug>

using namespace Mana;

Character::Character()
    : Being(OBJECT_CHARACTER)
    , mHairStyle(0)
    , mHairColor(0)
    , mCharacterSlot(0)
    , mLevel(0)
{
    if (!HairDB::instance()->isLoaded())
        connect(HairDB::instance(), SIGNAL(loaded()), SLOT(rebuildSprites()));
    if (!RaceDB::instance()->isLoaded())
        connect(RaceDB::instance(), SIGNAL(loaded()), SLOT(rebuildSprites()));
    if (!ItemDB::instance()->isLoaded())
        connect(ItemDB::instance(), SIGNAL(loaded()), SLOT(rebuildSprites()));
}

void Character::setEquipmentSlot(int slot, int itemId)
{
    if (mEquipmentSlots.value(slot) == itemId)
        return;

    QMap<int, int>::iterator it = mEquipmentSlots.find(slot);
    if (it != mEquipmentSlots.end()) {
        emit slotUnequipping(slot);
        it = mEquipmentSlots.erase(it);
        mSpriteList->removeSprite(SLOT_EQUIPMENT + slot);
    }

    if (itemId) {
        mEquipmentSlots[slot] = itemId;
        emit slotEquipped(slot, itemId);

        if (ItemDB::instance()->isLoaded()) {
            const ItemInfo *info = ItemDB::instance()->getInfo(itemId);
            if (!info) {
                qWarning() << Q_FUNC_INFO << "Tried to equip unknown item "
                           << itemId;
                return;
            }
            QVector<SpriteReference *> sprites = info->sprites(mGender);
            foreach (SpriteReference *sprite, sprites)
                mSpriteList->addSprite(SLOT_EQUIPMENT + slot, sprite);
        }
    }
}

void Character::setHairStyle(int style, int color)
{
    if (mHairStyle == style && mHairColor == color)
        return;

    mHairStyle = style;
    mHairColor = color;
    emit hairChanged();

    if (HairDB::instance()->isLoaded()) {
        if (const HairInfo *info = HairDB::instance()->getInfo(style))
            if (const SpriteReference *sprite = info->sprite(mGender))
                mSpriteList->setSprite(SLOT_HAIR, sprite);
    }
}

void Character::setGender(BeingGender gender)
{
    Being::setGender(gender);
    rebuildSprites();
}

void Character::rebuildSprites()
{
    mSpriteList->removeAll();

    if (HairDB::instance()->isLoaded()) {
        if (const HairInfo *hairInfo = HairDB::instance()->getInfo(mHairStyle))
            if (const SpriteReference *sprite = hairInfo->sprite(mGender))
                mSpriteList->addSprite(SLOT_HAIR, sprite);
    }

    if (RaceDB::instance()->isLoaded()) {
        const RaceInfo *raceInfo = RaceDB::instance()->races().at(0);
        if (const SpriteReference *sprite = raceInfo->sprite(mGender))
            mSpriteList->addSprite(SLOT_RACE, sprite);
    }

    if (ItemDB::instance()->isLoaded()) {
        QMapIterator<int, int> it(mEquipmentSlots);
        while (it.hasNext()) {
            it.next();
            const int slot = it.key();
            const int itemId = it.value();

            const ItemInfo *info = ItemDB::instance()->getInfo(itemId);
            if (!info) {
                qWarning() << Q_FUNC_INFO << "Unknown equipped item "
                           << itemId;
                continue;
            }
            QVector<SpriteReference *> sprites = info->sprites(mGender);
            foreach (SpriteReference *sprite, sprites)
                mSpriteList->addSprite(SLOT_EQUIPMENT + slot, sprite);
        }
    }
}
