/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Contributor(s): Jörg Müller.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

/** \file blender/makesrna/intern/rna_volume.c
 *  \ingroup RNA
 */

#include <stdlib.h>

#include "RNA_access.h"
#include "RNA_define.h"
#include "RNA_enum_types.h"

#include "rna_internal.h"

#include "DNA_volume_types.h"

#include "BLI_math_base.h"

#ifdef RNA_RUNTIME

#  include "BKE_volume.h"

#  include "DEG_depsgraph.h"

#  include "WM_types.h"
#  include "WM_api.h"

/* Updates */

static void rna_Volume_update_filepath(Main *UNUSED(bmain), Scene *UNUSED(scene), PointerRNA *ptr)
{
  Volume *volume = (Volume *)ptr->owner_id;
  BKE_volume_unload(volume);
  DEG_id_tag_update(&volume->id, ID_RECALC_COPY_ON_WRITE);
  WM_main_add_notifier(NC_GEOM | ND_DATA, volume);
}

/* Grid */

static void rna_VolumeGrid_name_get(PointerRNA *ptr, char *value)
{
  VolumeGrid *grid = ptr->data;
  strcpy(value, BKE_volume_grid_name(grid));
}

static int rna_VolumeGrid_name_length(PointerRNA *ptr)
{
  VolumeGrid *grid = ptr->data;
  return strlen(BKE_volume_grid_name(grid));
}

static int rna_VolumeGrid_channels_get(PointerRNA *ptr)
{
  const VolumeGrid *grid = ptr->data;
  return BKE_volume_grid_channels(grid);
}

static void rna_VolumeGrid_matrix_object_get(PointerRNA *ptr, float *value)
{
  VolumeGrid *grid = ptr->data;
  BKE_volume_grid_transform_matrix(grid, (float(*)[4])value);
}

static bool rna_VolumeGrid_is_loaded_get(PointerRNA *ptr)
{
  VolumeGrid *grid = ptr->data;
  return BKE_volume_grid_is_loaded(grid);
}

static bool rna_VolumeGrid_load(ID *id, VolumeGrid *grid)
{
  return BKE_volume_grid_load((Volume *)id, grid);
}

/* Grids Iterator */

static void rna_Volume_grids_begin(CollectionPropertyIterator *iter, PointerRNA *ptr)
{
  Volume *volume = ptr->data;
  int num_grids = BKE_volume_num_grids(volume);
  iter->internal.count.ptr = volume;
  iter->internal.count.item = 0;
  iter->valid = (iter->internal.count.item < num_grids);
}

static void rna_Volume_grids_next(CollectionPropertyIterator *iter)
{
  Volume *volume = iter->internal.count.ptr;
  int num_grids = BKE_volume_num_grids(volume);
  iter->internal.count.item++;
  iter->valid = (iter->internal.count.item < num_grids);
}

static void rna_Volume_grids_end(CollectionPropertyIterator *UNUSED(iter))
{
}

static PointerRNA rna_Volume_grids_get(CollectionPropertyIterator *iter)
{
  Volume *volume = iter->internal.count.ptr;
  const VolumeGrid *grid = BKE_volume_grid_get(volume, iter->internal.count.item);
  return rna_pointer_inherit_refine(&iter->parent, &RNA_VolumeGrid, (void *)grid);
}

static int rna_Volume_grids_length(PointerRNA *ptr)
{
  Volume *volume = ptr->data;
  return BKE_volume_num_grids(volume);
}

/* Active Grid */

static void rna_VolumeGrids_active_grid_index_range(
    PointerRNA *ptr, int *min, int *max, int *UNUSED(softmin), int *UNUSED(softmax))
{
  Volume *volume = (Volume *)ptr->data;
  int num_grids = BKE_volume_num_grids(volume);

  *min = 0;
  *max = max_ii(0, num_grids - 1);
}

static int rna_VolumeGrids_active_grid_index_get(PointerRNA *ptr)
{
  Volume *volume = (Volume *)ptr->data;
  int num_grids = BKE_volume_num_grids(volume);
  return clamp_i(volume->active_grid, 0, max_ii(num_grids - 1, 0));
}

static void rna_VolumeGrids_active_grid_index_set(PointerRNA *ptr, int value)
{
  Volume *volume = (Volume *)ptr->data;
  volume->active_grid = value;
}

/* Loading */

static bool rna_VolumeGrids_is_loaded_get(PointerRNA *ptr)
{
  Volume *volume = (Volume *)ptr->data;
  return BKE_volume_is_loaded(volume);
}

/* Error Message */

static void rna_VolumeGrids_error_message_get(PointerRNA *ptr, char *value)
{
  Volume *volume = (Volume *)ptr->data;
  strcpy(value, BKE_volume_grids_error_msg(volume));
}

static int rna_VolumeGrids_error_message_length(PointerRNA *ptr)
{
  Volume *volume = (Volume *)ptr->data;
  return strlen(BKE_volume_grids_error_msg(volume));
}

#else

static void rna_def_volume_grid(BlenderRNA *brna)
{
  StructRNA *srna;
  PropertyRNA *prop;

  srna = RNA_def_struct(brna, "VolumeGrid", NULL);
  RNA_def_struct_ui_text(srna, "Volume Grid", "3D volume grid");
  RNA_def_struct_ui_icon(srna, ICON_VOLUME_DATA);

  prop = RNA_def_property(srna, "name", PROP_STRING, PROP_NONE);
  RNA_def_property_clear_flag(prop, PROP_EDITABLE);
  RNA_def_property_string_funcs(
      prop, "rna_VolumeGrid_name_get", "rna_VolumeGrid_name_length", NULL);
  RNA_def_property_ui_text(prop, "Name", "Volume grid name");

  prop = RNA_def_property(srna, "channels", PROP_INT, PROP_UNSIGNED);
  RNA_def_property_clear_flag(prop, PROP_EDITABLE);
  RNA_def_property_int_funcs(prop, "rna_VolumeGrid_channels_get", NULL, NULL);
  RNA_def_property_ui_text(prop, "Channels", "Number of channels in voxel data");

  /* TODO: naming, clarification of what index space is. */
  prop = RNA_def_property(srna, "matrix_object", PROP_FLOAT, PROP_MATRIX);
  RNA_def_property_clear_flag(prop, PROP_EDITABLE);
  RNA_def_property_multi_array(prop, 2, rna_matrix_dimsize_4x4);
  RNA_def_property_float_funcs(prop, "rna_VolumeGrid_matrix_object_get", NULL, NULL);
  RNA_def_property_ui_text(
      prop, "Matrix Object", "Transformation from index space to world space");

  prop = RNA_def_property(srna, "is_loaded", PROP_BOOLEAN, PROP_NONE);
  RNA_def_property_clear_flag(prop, PROP_EDITABLE);
  RNA_def_property_boolean_funcs(prop, "rna_VolumeGrid_is_loaded_get", NULL);
  RNA_def_property_ui_text(prop, "Is Loaded", "Grid tree is loaded in memory");

  /* API */
  FunctionRNA *func;
  PropertyRNA *parm;

  func = RNA_def_function(srna, "load", "rna_VolumeGrid_load");
  RNA_def_function_ui_description(func, "Load grid tree from file");
  RNA_def_function_flag(func, FUNC_USE_SELF_ID);
  parm = RNA_def_boolean(func, "success", 0, "", "True if grid tree was successfully loaded");
  RNA_def_function_return(func, parm);

  func = RNA_def_function(srna, "unload", "BKE_volume_grid_unload");
  RNA_def_function_ui_description(
      func, "Unload grid tree and voxel data from memory, leaving only metadata");
}

static void rna_def_volume_grids(BlenderRNA *brna, PropertyRNA *cprop)
{
  StructRNA *srna;
  PropertyRNA *prop;

  RNA_def_property_srna(cprop, "VolumeGrids");
  srna = RNA_def_struct(brna, "VolumeGrids", NULL);
  RNA_def_struct_sdna(srna, "Volume");
  RNA_def_struct_ui_text(srna, "Volume Grids", "3D volume grids");

  prop = RNA_def_property(srna, "active_index", PROP_INT, PROP_UNSIGNED);
  RNA_def_property_int_funcs(prop,
                             "rna_VolumeGrids_active_grid_index_get",
                             "rna_VolumeGrids_active_grid_index_set",
                             "rna_VolumeGrids_active_grid_index_range");
  RNA_def_property_ui_text(prop, "Active Grid Index", "Index of active volume grid");

  prop = RNA_def_property(srna, "error_message", PROP_STRING, PROP_NONE);
  RNA_def_property_clear_flag(prop, PROP_EDITABLE);
  RNA_def_property_string_funcs(
      prop, "rna_VolumeGrids_error_message_get", "rna_VolumeGrids_error_message_length", NULL);
  RNA_def_property_ui_text(
      prop, "Error Message", "If loading grids failed, error message with details");

  prop = RNA_def_property(srna, "is_loaded", PROP_BOOLEAN, PROP_NONE);
  RNA_def_property_clear_flag(prop, PROP_EDITABLE);
  RNA_def_property_boolean_funcs(prop, "rna_VolumeGrids_is_loaded_get", NULL);
  RNA_def_property_ui_text(prop, "Is Loaded", "List of grids and metadata are loaded in memory");

  /* API */
  FunctionRNA *func;
  PropertyRNA *parm;

  func = RNA_def_function(srna, "load", "BKE_volume_load");
  RNA_def_function_ui_description(func, "Load list of grids and metadata from file");
  RNA_def_function_flag(func, FUNC_USE_MAIN);
  parm = RNA_def_boolean(func, "success", 0, "", "True if grid list was successfully loaded");
  RNA_def_function_return(func, parm);

  func = RNA_def_function(srna, "unload", "BKE_volume_unload");
  RNA_def_function_ui_description(func, "Unload all grid and voxel data from memory");
}

static void rna_def_volume(BlenderRNA *brna)
{
  StructRNA *srna;
  PropertyRNA *prop;

  srna = RNA_def_struct(brna, "Volume", "ID");
  RNA_def_struct_ui_text(srna, "Volume", "Volume data-block for 3D volume grids");
  RNA_def_struct_ui_icon(srna, ICON_VOLUME_DATA);

  prop = RNA_def_property(srna, "filepath", PROP_STRING, PROP_FILEPATH);
  RNA_def_property_ui_text(prop, "File Path", "Volume sample file used by this Volume data-block");
  RNA_def_property_update(prop, 0, "rna_Volume_update_filepath");

  prop = RNA_def_property(srna, "packed_file", PROP_POINTER, PROP_NONE);
  RNA_def_property_pointer_sdna(prop, NULL, "packedfile");
  RNA_def_property_ui_text(prop, "Packed File", "");

  prop = RNA_def_property(srna, "grids", PROP_COLLECTION, PROP_NONE);
  RNA_def_property_struct_type(prop, "VolumeGrid");
  RNA_def_property_ui_text(prop, "Grids", "3D volume grids");
  RNA_def_property_collection_funcs(prop,
                                    "rna_Volume_grids_begin",
                                    "rna_Volume_grids_next",
                                    "rna_Volume_grids_end",
                                    "rna_Volume_grids_get",
                                    "rna_Volume_grids_length",
                                    NULL,
                                    NULL,
                                    NULL);
  rna_def_volume_grids(brna, prop);

  /* materials */
  prop = RNA_def_property(srna, "materials", PROP_COLLECTION, PROP_NONE);
  RNA_def_property_collection_sdna(prop, NULL, "mat", "totcol");
  RNA_def_property_struct_type(prop, "Material");
  RNA_def_property_ui_text(prop, "Materials", "");
  RNA_def_property_srna(prop, "IDMaterials"); /* see rna_ID.c */
  RNA_def_property_collection_funcs(
      prop, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "rna_IDMaterials_assign_int");

  /* common */
  rna_def_animdata_common(srna);
}

void RNA_def_volume(BlenderRNA *brna)
{
  rna_def_volume_grid(brna);
  rna_def_volume(brna);
}

#endif