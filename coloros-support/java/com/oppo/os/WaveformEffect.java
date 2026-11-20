/*
 * SPDX-FileCopyrightText: 2026 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

package com.oppo.os;

public class WaveformEffect {
    private int mEffectType;

    public static class Builder {
        private int mEffectType;

        public Builder setEffectType(int effectType) {
            mEffectType = effectType;
            return this;
        }

        public WaveformEffect build() {
            WaveformEffect effect = new WaveformEffect();
            effect.mEffectType = mEffectType;
            return effect;
        }
    }
}
