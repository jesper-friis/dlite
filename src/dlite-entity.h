#ifndef _DLITE_ENTITY_H
#define _DLITE_ENTITY_H

/**
  @file
  @brief API for instances and entities.

  There is only data and transformations.  All data are instances of
  their metadata and metadata are instances of their meta-metadata,
  etc...


  Instances
  ---------
  Instances are uniquely defined by their UUID.  In addition they may
  have a unique name, which we here refers to as their URI.  For
  instance, for an Entiry, the concatenated "namespace/name/version"
  string is its URI.  If an instance has an URI, the UUID is generated
  from it (as a version 5 SHA1-based UUID using the DNS namespace),
  otherwise a random UUID is generated (as a version 4 UUID).

  The memory allocated for each instance, may be casted to a struct
  starting with the members defined by the DLiteInstance_HEAD macro,
  which contains the UUID and (optionally) URI identifying the
  instance as well as a pointer th the metadata describing the
  instance.  This header is followed by:

    - the size of each dimension (size_t)
    - the value of each property
    - relations (optional, data instances and entities have no relations)

  The memory offset from a pointer to the instance to the first
  element in the arrays of dimension sizes and properties are given by
  the `dimoffset` and `propoffset` members of the metadata, respectively.


  Metadata
  --------
  Since all metadata is an instance of its meta-metadata, it is also
  an instance.  The DLiteMeta_HEAD, which is common to all metadata,
  therefore starts with DLiteInstance_HEAD.

  It can be shown that DLite metadata can describe itself.  In
  principle we can therefore follow the `meta` to higher and higher
  levels of abstraction, until we reach the basic metadata schema,
  which has its `meta` member set to NULL.  However, at least
  initially, we will allow to set the `meta` member of any DLiteMeta
  to NULL, indicating that we are not interested in the next level of
  abstraction.  The concequence of setting the `meta` member to NULL
  for a metadata object, is that we can not access it via the instance
  API.

  The metadata for normal data instances are called Entities and are
  represented by DLiteEntity.
*/

#include "boolean.h"
#include "dlite.h"
#include "dlite-type.h"
#include "dlite-storage.h"


/**
  Initial segment of all DLite instances.
*/
#define DLiteInstance_HEAD                                                \
  char uuid[DLITE_UUID_LENGTH+1]; /*!< UUID for this data instance. */    \
  const char *uri;                /*!< Unique name or uri to the data */  \
                                  /*   instance.  Can be NULL. */         \
  struct _DLiteMeta *meta;        /*!< Pointer to the metadata descri- */ \
                                  /*!< bing this instance. */


/**
  Initial segment of all DLite metadata.  With instance we here refer
  to the dataset described by this metadata.

  For convinience we include `ndimensions`, `nproperties` and
  `nrelations` in the header of all MetaData, so we don't have to
  check the existence of these dimensions in the meta-metadata.  A
  consequence is that all meta-metadata must include these as the
  first 3 dimensions, regardless of whether they actually are used (as
  in the case for Entity, which don't have relations).
*/
#define DLiteMeta_HEAD                                                  \
  DLiteInstance_HEAD                                                    \
  const char *description;  /*!< Description of this metadata. */	\
									\
  /* internal data */							\
  size_t size;         /*!< Size of instance memory. */                 \
  size_t dimoffset;    /*!< Memory offset of dimensions in instance. */ \
  size_t *propoffsets; /*!< Memory offset of each property value. */    \
                       /*   NULL if instance is metadata. */            \
  size_t reloffset;    /*!< Memory offset of relations  in instance. */ \
  int refcount;        /*!< Number of instances referring to this */    \
                       /*   metadata. */                                \
                                                                        \
  /* schema_properties */                                               \
  DLiteDimension *dimensions;     /*!< Array of dimensions. */          \
  DLiteBaseProperty **properties; /*!< Array of property pointers. */	\
  DLiteTriplet *relations;        /*!< Array of relations. */           \
                                                                        \
  /* schema_dimensions */                                               \
  /* We place the dimensions at the end, in case more are needed */     \
  size_t ndimensions;  /*!< Number of dimensions in instance. */        \
  size_t nproperties;  /*!< Number of properties in instance. */        \
  size_t nrelations;   /*!< Number of relations in instance. */


/**
  Initial segment of all DLite Properties.

  E.g. if we have dimensions ["M", "N"] and dims is [1, 1, 0], it
  means that the data described by this property has dimensions ["N",
  "N", "M"].
 */
#define DLiteProperty_HEAD                                              \
  char *name;         /*!< Name of this property. */                    \
  DLiteType type;     /*!< Type of the described data. */               \
  size_t size;        /*!< Size of one data element. */                 \
  int ndims;          /*!< Number of dimension of the described */      \
                      /*   data. */                                     \
  int *dims;          /*!< Array of dimension indices. */               \
  char *description;  /*!< Human described of the described data. */



/**
  Base definition of a DLite instance, that all instance (and
  metadata) objects can be cast to.  Is never actually
  instansiated.
*/
struct _DLiteInstance {
  DLiteInstance_HEAD
};


/** Dimension */
typedef struct {
  char *name;         /*!< Name of this dimension. */
  char *description;  /*!< Description of this dimension. */
} DLiteDimension;


/**
  Base definition of a DLite property that all properties can be cast to.
*/
typedef struct _DLiteBaseProperty{
  DLiteProperty_HEAD
} DLiteBaseProperty;


/**
  Entity property
*/
typedef struct _DLiteProperty {
  DLiteProperty_HEAD
  char *unit;         /*!< Unit of the described data. */
} DLiteProperty;


/**
  Base definition of a DLite metadata, that all metadata objects can
  be cast to.

  It may be instansiated, e.g. by the basic metadata schema.
*/
typedef struct _DLiteMeta {
  DLiteMeta_HEAD
} DLiteMeta;


/**
  A DLite entity.

  This is the metadata for standard data objects.  Entities will
  typically have it `meta` member set to NULL.  This works, since
  entities have their own API which know about their structure.
*/
struct _DLiteEntity {
  DLiteMeta_HEAD
};


/**
  A DLite Collection.

  Collections are a special type of instances that hold a set of
  instances and relations between them.

  In the current implementation, we allow the `meta` field to be NULL.

  A set of pre-defined relations are used to manage the collection itself.
  In order to not distinguish these relations from user-defined relations,
  their predicate are prefixed with a single underscore.  The pre-defined
  relations are:

  subject | predicate   | object
  ------- | ----------- | ----------
  label   | "_is-a"     | "Instance"
  label   | "_has-uuid" | uuid
  label   | "_has-meta" | uri
  label   | "_dim-map"  | instdim:colldim

  The "_dim-map" relation maps the name of a dimension in an
  instance (`instdim`) to a common dimension in the collection
  (`colldim`).  The object is the concatenation of `instdim`
  and `colldim` separated by a colon.
*/
typedef struct _DLiteCollection {
  DLiteInstance_HEAD
  size_t ninstances;          /*!< Number of instances. */
  size_t ntriplets;           /*!< Number of relations. */

  char **labels;              /*!< Array of instances. */
  DLiteTriplet *triplets;     /*!< Array of relation triplets. */
  char **dimnames;            /*!< Name of each (common) dimension. */
  int *dimsizes;              /*!< Size of each (common) dimension. */
} DLiteCollection;


/********************************************************************
 *  Instances
********************************************************************/

/**
  Returns a new uninitialised dlite instance of Entiry \a meta with
  dimensions \a dims.  Memory for all properties is allocated and set
  to zero.  The lengths of \a dims is found in `meta->ndims`.

  Increases the reference count of \a meta.

  On error, NULL is returned.
 */
DLiteInstance *dlite_instance_create(DLiteEntity *meta, size_t *dims,
                                     const char *id);

/**
  Free's an instance and all arrays associated with dimensional properties.
  Decreases the reference count of the associated metadata.
 */
void dlite_instance_free(DLiteInstance *inst);


/**
  Loads instance identified by \a id from storage \a s and returns a
  new and fully initialised dlite instance.

  On error, NULL is returned.
 */
DLiteInstance *dlite_instance_load(DLiteStorage *s, const char *id,
				   DLiteEntity *entity);


/**
  Saves instance \a inst to storage \a s.  Returns non-zero on error.
 */
int dlite_instance_save(DLiteStorage *s, const DLiteInstance *inst);


/**
  Returns size of dimension \a i or -1 on error.
 */
int dlite_instance_get_dimension_size_by_index(const DLiteInstance *inst,
                                               size_t i);

/**
  Returns a pointer to data corresponding to property with index \a i
  or NULL on error.
 */
const void *dlite_instance_get_property_by_index(const DLiteInstance *inst,
						 size_t i);

/**
  Sets property \a i to the value pointed to by \a ptr.
  Returns non-zero on error.
*/
int dlite_instance_set_property_by_index(DLiteInstance *inst, size_t i,
					 const void *ptr);

/**
  Returns number of dimensions of property with index \a i or -1 on error.
 */
int dlite_instance_get_property_ndims_by_index(const DLiteInstance *inst,
					       size_t i);

/**
  Returns size of dimension \a j in property \a i or -1 on error.
 */
int dlite_instance_get_property_dimsize_by_index(const DLiteInstance *inst,
						 size_t i, size_t j);

/**
  Returns size of dimension \a i or -1 on error.
 */
int dlite_instance_get_dimension_size(const DLiteInstance *inst,
                                      const char *name);

/**
  Returns a pointer to data corresponding to \a name or NULL on error.
 */
const void *dlite_instance_get_property(const DLiteInstance *inst,
					const char *name);

/**
  Copies memory pointed to by \a ptr to property \a name.
  Returns non-zero on error.
*/
int dlite_instance_set_property(DLiteInstance *inst, const char *name,
				const void *ptr);

/**
  Returns number of dimensions of property  \a name or -1 on error.
*/
int dlite_instance_get_property_ndims(const DLiteInstance *inst,
				      const char *name);

/**
  Returns size of dimension \a j of property \a name or NULL on error.
*/
size_t dlite_instance_get_property_dimssize(const DLiteInstance *inst,
					    const char *name, size_t j);


/********************************************************************
 *  Entities
********************************************************************/

/**
  Returns a new Entity created from the given arguments.
 */
DLiteEntity *
dlite_entity_create(const char *uri, const char *description,
		    size_t ndimensions, const DLiteDimension *dimensions,
		    size_t nproperties, const DLiteProperty *properties);


/**
  Increase reference count to Entity.
 */
void dlite_entity_incref(DLiteEntity *entity);

/**
  Decrease reference count to Entity.  If the reference count reaches
  zero, the Entity is free'ed.
 */
void dlite_entity_decref(DLiteEntity *entity);

/**
  Free's all memory used by \a entity and clear all data.
 */
void dlite_entity_clear(DLiteEntity *entity);

/**
  Returns a new Entity loaded from storage \a s.  The \a s may be either
  an URI to the Entity (typically of the form "namespace/version/name")
  or an UUID.

  Returns NULL on error.
 */
DLiteEntity *dlite_entity_load(const DLiteStorage *s, const char *id);


/**
  Saves an Entity to storage \a s.  Returns non-zero on error.
 */
int dlite_entity_save(DLiteStorage *s, const DLiteEntity *entity);


/*
  Returns a pointer to property with index \a i or NULL on error.
 */
const DLiteProperty *
dlite_entity_get_property_by_index(const DLiteEntity *entity, size_t i);

/*
  Returns a pointer to property named \a name or NULL on error.
 */
const DLiteProperty *dlite_entity_get_property(const DLiteEntity *entity,
					       const char *name);


/********************************************************************
 *  Generic metadata
 *
 *  These functions are mainly used internally or by code generators.
 *  Do not waist time on them...
 ********************************************************************/

/**
  Initialises internal properties of \a meta.  This function should
  not be called before the non-internal properties has been initialised.

  The \a ismeta argument indicates whether the instance described by
  `meta` is metadata itself.

  Returns non-zero on error.
 */
int dlite_meta_postinit(DLiteMeta *meta, bool ismeta);

/**
  Free's all memory used by \a meta and clear all data.
 */
void dlite_meta_clear(DLiteMeta *meta);


/**
  Increase reference count to meta-metadata.
 */
void dlite_meta_incref(DLiteMeta *meta);

/**
  Decrease reference count to meta-metadata.  If the reference count reaches
  zero, the meta-metadata is free'ed.
 */
void dlite_meta_decref(DLiteMeta *meta);


/**
  Returns index of dimension named \a name or -1 on error.
 */
int dlite_meta_get_dimension_index(const DLiteMeta *meta, const char *name);

/**
  Returns index of property named `name` or -1 on error.
 */
int dlite_meta_get_property_index(const DLiteMeta *meta, const char *name);

/**
  Returns a pointer to property named \a name or NULL on error.
 */
const DLiteBaseProperty *dlite_meta_get_property(const DLiteMeta *meta,
						 const char *name);

#endif /* _DLITE_ENTITY_H */