/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <stdbool.h>

typedef enum UnitFilePresetMode UnitFilePresetMode;
typedef enum InstallChangeType InstallChangeType;
typedef enum UnitFileFlags UnitFileFlags;
typedef enum InstallMode InstallMode;
typedef struct InstallChange InstallChange;
typedef struct UnitFileList UnitFileList;
typedef struct InstallInfo InstallInfo;

#include "hashmap.h"
#include "macro.h"
#include "path-lookup.h"
#include "strv.h"
#include "unit-file.h"
#include "unit-name.h"

enum UnitFilePresetMode {
        UNIT_FILE_PRESET_FULL,
        UNIT_FILE_PRESET_ENABLE_ONLY,
        UNIT_FILE_PRESET_DISABLE_ONLY,
        _UNIT_FILE_PRESET_MAX,
        _UNIT_FILE_PRESET_INVALID = -EINVAL,
};

/* This enum type is anonymous, since we usually store it in an 'int', as we overload it with negative errno
 * values. */
enum {
        INSTALL_CHANGE_SYMLINK,
        INSTALL_CHANGE_UNLINK,
        INSTALL_CHANGE_IS_MASKED,
        INSTALL_CHANGE_IS_MASKED_GENERATOR,
        INSTALL_CHANGE_IS_DANGLING,
        INSTALL_CHANGE_DESTINATION_NOT_PRESENT,
        INSTALL_CHANGE_AUXILIARY_FAILED,
        _INSTALL_CHANGE_MAX,
        _INSTALL_CHANGE_INVALID = -EINVAL,
};

enum UnitFileFlags {
        UNIT_FILE_RUNTIME                  = 1 << 0, /* Public API via DBUS, do not change */
        UNIT_FILE_FORCE                    = 1 << 1, /* Public API via DBUS, do not change */
        UNIT_FILE_PORTABLE                 = 1 << 2, /* Public API via DBUS, do not change */
        UNIT_FILE_DRY_RUN                  = 1 << 3,
        UNIT_FILE_IGNORE_AUXILIARY_FAILURE = 1 << 4,
        _UNIT_FILE_FLAGS_MASK_PUBLIC = UNIT_FILE_RUNTIME|UNIT_FILE_PORTABLE|UNIT_FILE_FORCE,
};

/* change_or_errno can be either one of the INSTALL_CHANGE_SYMLINK, INSTALL_CHANGE_UNLINK, … listed above, or
 * a negative errno value.
 *
 * If source is specified, it should be the contents of the path symlink. In case of an error, source should
 * be the existing symlink contents or NULL. */
struct InstallChange {
        int change_or_errno; /* INSTALL_CHANGE_SYMLINK, … if positive, errno if negative */
        char *path;
        char *source;
};

static inline bool install_changes_have_modification(const InstallChange* changes, size_t n_changes) {
        for (size_t i = 0; i < n_changes; i++)
                if (IN_SET(changes[i].change_or_errno, INSTALL_CHANGE_SYMLINK, INSTALL_CHANGE_UNLINK))
                        return true;
        return false;
}

struct UnitFileList {
        char *path;
        UnitFileState state;
};

enum InstallMode {
        INSTALL_MODE_REGULAR,
        INSTALL_MODE_LINKED,
        INSTALL_MODE_ALIAS,
        INSTALL_MODE_MASKED,
        _INSTALL_MODE_MAX,
        _INSTALL_MODE_INVALID = -EINVAL,
};

struct InstallInfo {
        char *name;
        char *path;
        char *root;

        char **aliases;
        char **wanted_by;
        char **required_by;
        char **also;

        char *default_instance;
        char *symlink_target;

        InstallMode install_mode;
        bool auxiliary;
};

int unit_file_enable(
                LookupScope scope,
                UnitFileFlags flags,
                const char *root_dir,
                char **names_or_paths,
                InstallChange **changes,
                size_t *n_changes);
int unit_file_disable(
                LookupScope scope,
                UnitFileFlags flags,
                const char *root_dir,
                char **names,
                InstallChange **changes,
                size_t *n_changes);
int unit_file_reenable(
                LookupScope scope,
                UnitFileFlags flags,
                const char *root_dir,
                char **names_or_paths,
                InstallChange **changes,
                size_t *n_changes);
int unit_file_preset(
                LookupScope scope,
                UnitFileFlags flags,
                const char *root_dir,
                char **names,
                UnitFilePresetMode mode,
                InstallChange **changes,
                size_t *n_changes);
int unit_file_preset_all(
                LookupScope scope,
                UnitFileFlags flags,
                const char *root_dir,
                UnitFilePresetMode mode,
                InstallChange **changes,
                size_t *n_changes);
int unit_file_mask(
                LookupScope scope,
                UnitFileFlags flags,
                const char *root_dir,
                char **names,
                InstallChange **changes,
                size_t *n_changes);
int unit_file_unmask(
                LookupScope scope,
                UnitFileFlags flags,
                const char *root_dir,
                char **names,
                InstallChange **changes,
                size_t *n_changes);
int unit_file_link(
                LookupScope scope,
                UnitFileFlags flags,
                const char *root_dir,
                char **files,
                InstallChange **changes,
                size_t *n_changes);
int unit_file_revert(
                LookupScope scope,
                const char *root_dir,
                char **names,
                InstallChange **changes,
                size_t *n_changes);
int unit_file_set_default(
                LookupScope scope,
                UnitFileFlags flags,
                const char *root_dir,
                const char *name,
                InstallChange **changes,
                size_t *n_changes);
int unit_file_get_default(
                LookupScope scope,
                const char *root_dir,
                char **name);
int unit_file_add_dependency(
                LookupScope scope,
                UnitFileFlags flags,
                const char *root_dir,
                char **names,
                const char *target,
                UnitDependency dep,
                InstallChange **changes,
                size_t *n_changes);

int unit_file_lookup_state(
                LookupScope scope,
                const LookupPaths *paths,
                const char *name,
                UnitFileState *ret);

int unit_file_get_state(LookupScope scope, const char *root_dir, const char *filename, UnitFileState *ret);
int unit_file_exists(LookupScope scope, const LookupPaths *paths, const char *name);

int unit_file_get_list(LookupScope scope, const char *root_dir, Hashmap *h, char **states, char **patterns);
Hashmap* unit_file_list_free(Hashmap *h);

int install_changes_add(InstallChange **changes, size_t *n_changes, int type, const char *path, const char *source);
void install_changes_free(InstallChange *changes, size_t n_changes);
void install_changes_dump(int r, const char *verb, const InstallChange *changes, size_t n_changes, bool quiet);

int unit_file_verify_alias(
                const InstallInfo *info,
                const char *dst,
                char **ret_dst,
                InstallChange **changes,
                size_t *n_changes);

typedef struct UnitFilePresetRule UnitFilePresetRule;

typedef struct {
        UnitFilePresetRule *rules;
        size_t n_rules;
        bool initialized;
} UnitFilePresets;

void unit_file_presets_freep(UnitFilePresets *p);
int unit_file_query_preset(LookupScope scope, const char *root_dir, const char *name, UnitFilePresets *cached);

const char *unit_file_state_to_string(UnitFileState s) _const_;
UnitFileState unit_file_state_from_string(const char *s) _pure_;
/* from_string conversion is unreliable because of the overlap between -EPERM and -1 for error. */

const char *install_change_to_string(int s) _const_;
int install_change_from_string(const char *s) _pure_;

const char *unit_file_preset_mode_to_string(UnitFilePresetMode m) _const_;
UnitFilePresetMode unit_file_preset_mode_from_string(const char *s) _pure_;
