#ifndef _DLITE_PLUGINS_H
#define _DLITE_PLUGINS_H

/**
  @file
  @brief Common API for all plugins.
*/

#include "dlite-storage.h"


/** Initial segment of all DLiteStorage plugin data structures. */
#define DLiteStorage_HEAD                                               \
  struct _DLitePlugin *api; /*!< Pointer to plugin api */               \
  char *uri;                /*!< URI passed to dlite_storage_open() */  \
  char *options;            /*!< Options passed to dlite_storage_open() */ \
  int writable;             /*!< Whether storage is writable */ \
  DLiteIDFlag idflag;       /*!< How to handle instance id's */


/** Initial segment of all DLiteDataModel plugin data structures. */
#define DLiteDataModel_HEAD                                     \
  struct _DLitePlugin *api; /*!< Pointer to plugin api */       \
  DLiteStorage *s;          /*!< Pointer to storage */          \
  char uuid[37];            /*!< UUID for the stored data */


/** Base definition of a DLite storage, that all plugin storage
    objects can be cast to.  Is never actually instansiated. */
struct _DLiteStorage {
  DLiteStorage_HEAD
};

/** Base definition of a DLite data model, that all plugin data model
    objects can be cast to.  Is never actually instansiated. */
struct _DLiteDataModel {
  DLiteDataModel_HEAD
};


/*
  FIXME - plugins should be dynamically loadable

  See http://gernotklingler.com/blog/creating-using-shared-libraries-different-compilers-different-operating-systems/

  (sodyll)[https://github.com/petervaro/sodyll] provides a nice
  portable header.  Unfortunately it is GPL. Can we find something
  similar?
 */


/**
 * @name Required api
 * Signatures of function that must be defined by all plugins.
 * @{
 */

/**
  Opens `uri` and returns a newly created storage for it.

  The `options` may vary between plugins.  Typical values includes:

    - rw : read and write: open existing storage or create new (default)
    - r  : read-only: open existing storage for read-only
    - a  : append: open existing storage for read and write
    - w  : write: truncate existing storage or create new

  Returns NULL on error.
 */
typedef DLiteStorage *(*Open)(const char *uri, const char *options);


/**
  Closes storage `s`.  Returns non-zero on error.
 */
typedef int (*Close)(DLiteStorage *s);


/**
  Creates a new datamodel for storage `s`.

  If `uuid` exists in the root of `s`, the datamodel should describe the
  corresponding instance.

  Otherwise (if `s` is writable), a new instance described by the
  datamodel should be created in `s`.

  Returns the new datamodel or NULL on error.
 */
typedef DLiteDataModel *(*DataModel)(const DLiteStorage *s, const char *uuid);


/**
  Frees all memory associated with datamodel `d`.
 */
typedef int (*DataModelFree)(DLiteDataModel *d);


/**
  Returns a pointer to newly malloc'ed metadata uri for datamodel `d`
  or NULL on error.
 */
typedef char *(*GetMetaURI)(const DLiteDataModel *d);


/**
  Returns the size of dimension `name` or 0 on error.
 */
/* FIXME - zero may be a valid dimension size.  Change from size_t to a
   signed integer and return -1 on error. */
typedef int (*GetDimensionSize)(const DLiteDataModel *d, const char *name);


/**
  Copies property `name` to memory pointed to by `ptr`.

  The expected type, size, number of dimensions and size of each
  dimension of the memory is described by `type`, `size`, `ndims` and
  `dims`, respectively.

  Returns non-zero on error.
 */
typedef int (*GetProperty)(const DLiteDataModel *d, const char *name,
                           void *ptr, DLiteType type, size_t size,
                           size_t ndims, const size_t *dims);


/** @} */

/**
 * @name Optional api
 * Signatures of function that are optional for the plugins to define.
 * @{
 */

/**
  Returns a newly malloc'ed NULL-terminated array of (malloc'ed)
  string pointers to the UUID's of all instances in storage `s`.

  The caller is responsible to free the returned array.

  Returns NULL on error.
 */
typedef char **(*GetUUIDs)(const DLiteStorage *s);


/**
  Sets the metadata uri in datamodel `d` to `uri`.  Returns non-zero on error.
 */
typedef int (*SetMetaURI)(DLiteDataModel *d, const char *uri);


/**
  Sets the size of dimension `name`.  Returns non-zero on error.
 */
typedef int (*SetDimensionSize)(DLiteDataModel *d, const char *name,
                                size_t size);


/**
  Sets property `name` to the memory pointed to by `ptr`.

  The expected type, size, number of dimensions and size of each
  dimension of the memory is described by `type`, `size`, `ndims` and
  `dims`, respectively.

  Returns non-zero on error.
 */
typedef int (*SetProperty)(DLiteDataModel *d, const char *name,
                           const void *ptr, DLiteType type, size_t size,
                           size_t ndims, const size_t *dims);


/**
  Returns a positive value if dimension `name` is defined, zero if it
  isn't and a negative value on error.
 */
typedef int (*HasDimension)(const DLiteDataModel *d, const char *name);


/**
  Returns a positive value if property `name` is defined, zero if it
  isn't and a negative value on error.
 */
typedef int (*HasProperty)(const DLiteDataModel *d, const char *name);


/**
  If the uuid was generated from a unique name, return a pointer to a
  newly malloc'ed string with this name.  Otherwise NULL is returned.
*/
typedef char *(*GetDataName)(const DLiteDataModel *d);


/**
  Gives the instance a name.  This function should only be called
  if the uuid was generated from `name`.
  Returns non-zero on error.
*/
typedef int (*SetDataName)(DLiteDataModel *d, const char *name);

/** @} */


/**
 * @name Specialised api for single-entity storage
 * Signatures of specialised functions for loading/saving a storage
 * containing only a single entity.
 *
 * These functions are probably only relevant for the JSON plugin to
 * define for loading/storing standard entity json-files.
 * @{
 */

/**
  Returns a new entity loaded from storage `s` with single-entity layout.
 */
/* FIXME - remove uuid argument, since it is not needed */
typedef DLiteEntity *(*GetEntity)(const DLiteStorage *s, const char *uuid);

/**
  Stores entity `e` to storage `s`, using the single-entity layout.
 */
typedef int (*SetEntity)(DLiteStorage *s, const DLiteEntity *e);

/** @} */


/** Struct with the name and pointers to function for a plugin. All
    plugins should define themselves by defining an intance of DLitePlugin. */
typedef struct _DLitePlugin {
  /* Name of plugin */
  const char *       name;             /*!< Name of plugin */

  /* Minimum api */
  Open               open;             /*!< Open storage */
  Close              close;            /*!< Close storage */

  DataModel          dataModel;        /*!< Creates new data model */
  DataModelFree      dataModelFree;    /*!< Frees a data model */

  GetMetaURI         getMetaURI;       /*!< Returns uri to metadata */
  GetDimensionSize   getDimensionSize; /*!< Returns size of dimension */
  GetProperty        getProperty;      /*!< Gets value of property */

  /* Optional api */
  GetUUIDs           getUUIDs;         /*!< Returns all UUIDs in storage */

  SetMetaURI         setMetaURI;       /*!< Sets metadata uri */
  SetDimensionSize   setDimensionSize; /*!< Sets size of dimension */
  SetProperty        setProperty;      /*!< Sets value of property */

  HasDimension       hasDimension;     /*!< Checks for dimension name */
  HasProperty        hasProperty;      /*!< Checks for property name */

  GetDataName        getDataName;      /*!< Returns name of instance */
  SetDataName        setDataName;      /*!< Assigns name to instance */

  /* Specialised api */
  GetEntity          getEntity;        /*!< Returns a new Entity from storage */
  SetEntity          setEntity;        /*!< Stores an Entity */
} DLitePlugin;


#endif /* _DLITE_PLUGINS_H */
