/*
   Copyright (c) 2016, The CyanogenMod Project
   Copyright (c) 2019-2020, The LineageOS Project

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cstdlib>
#include <fstream>
#include <string.h>
#include <sys/sysinfo.h>
#include <unistd.h>

#include <android-base/properties.h>
#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>
#include <init_lmk.h>

#include "vendor_init.h"
#include "property_service.h"

#define PARTIAL_STALL_MS_PROP "ro.lmk.psi_partial_stall_ms"
#define COMPLETE_STALL_MS_PROP "ro.lmk.psi_complete_stall_ms"
#define THRASHING_LIMIT_PROP "ro.lmk.thrashing_limit"
#define THRASHING_LIMIT_DECAY_PROP "ro.lmk.thrashing_limit_decay"
#define SWAP_UTIL_MAX_PROP "ro.lmk.swap_util_max"

char const *heapstartsize;
char const *heapgrowthlimit;
char const *heapsize;
char const *heapminfree;
char const *heapmaxfree;
char const *heaptargetutilization;

void check_device()
{
    struct sysinfo sys;

    sysinfo(&sys);

    if (sys.totalram > 5072ull * 1024 * 1024) {
        // from - phone-xhdpi-6144-dalvik-heap.mk
        heapstartsize = "16m";
        heapgrowthlimit = "256m";
        heapsize = "512m";
        heaptargetutilization = "0.5";
        heapminfree = "8m";
        heapmaxfree = "32m";
    } else if (sys.totalram > 3072ull * 1024 * 1024) {
        // from - phone-xxhdpi-4096-dalvik-heap.mk
        heapstartsize = "8m";
        heapgrowthlimit = "256m";
        heapsize = "512m";
        heaptargetutilization = "0.6";
        heapminfree = "8m";
        heapmaxfree = "16m";
    } else {
        // from - phone-xhdpi-2048-dalvik-heap.mk
        heapstartsize = "8m";
        heapgrowthlimit = "192m";
        heapsize = "512m";
        heaptargetutilization = "0.75";
        heapminfree = "512k";
        heapmaxfree = "8m";
    }
}

void property_override(char const prop[], char const value[], bool add = true)
{
    auto pi = (prop_info *) __system_property_find(prop);

    if (pi != nullptr) {
        __system_property_update(pi, value, strlen(value));
    } else if (add) {
        __system_property_add(prop, strlen(prop), value, strlen(value));
    }
}

void set_avoid_gfxaccel_config() {
    struct sysinfo sys;
    sysinfo(&sys);

    if (sys.totalram <= 3072ull * 1024 * 1024) {
        // Reduce memory footprint
        property_override("ro.config.avoid_gfx_accel", "true");
    }
}

// lmk parameters for
// 6GB variant
static const lmk_info_t lmk_6144 = {
    .psi_partial_stall_ms = "70",
    .psi_complete_stall_ms = "700",
    .thrashing_limit = "100",
    .thrashing_limit_decay = "10",
    .swap_util_max = "90",
};

// 4GB variant
static const lmk_info_t lmk_4096 = {
    .psi_partial_stall_ms = "130",
    .psi_complete_stall_ms = "700",
    .thrashing_limit = "55",
    .thrashing_limit_decay = "35",
    .swap_util_max = "90",
};

// 3GB variant
static const lmk_info_t lmk_3072 = {
    .psi_partial_stall_ms = "200",
    .psi_complete_stall_ms = "700",
    .thrashing_limit = "55",
    .thrashing_limit_decay = "35",
    .swap_util_max = "90",
};

void set_lmk() {
    struct sysinfo sys;
    const lmk_info_t *lmk_values;

    sysinfo(&sys);

    if (sys.totalram > 5072ull * 1024 * 1024)
        lmk_values = &lmk_6144;
    else if (sys.totalram > 3072ull * 1024 * 1024)
        lmk_values = &lmk_4096;
    else
        lmk_values = &lmk_3072;

    property_override(PARTIAL_STALL_MS_PROP, lmk_values->psi_partial_stall_ms);
    property_override(COMPLETE_STALL_MS_PROP, lmk_values->psi_complete_stall_ms);
    property_override(THRASHING_LIMIT_PROP, lmk_values->thrashing_limit);
    property_override(THRASHING_LIMIT_DECAY_PROP, lmk_values->thrashing_limit_decay);
    property_override(SWAP_UTIL_MAX_PROP, lmk_values->swap_util_max);
}

void vendor_load_properties()
{
    check_device();
    set_avoid_gfxaccel_config();
    set_lmk();
    // Reinitialize lmkd
    property_override("lmkd.reinit", "1");

    property_override("dalvik.vm.heapstartsize", heapstartsize);
    property_override("dalvik.vm.heapgrowthlimit", heapgrowthlimit);
    property_override("dalvik.vm.heapsize", heapsize);
    property_override("dalvik.vm.heaptargetutilization", heaptargetutilization);
    property_override("dalvik.vm.heapminfree", heapminfree);
    property_override("dalvik.vm.heapmaxfree", heapmaxfree);
}
