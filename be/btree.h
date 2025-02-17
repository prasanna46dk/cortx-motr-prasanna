/* -*- C -*- */
/*
 * Copyright (c) 2013-2021 Seagate Technology LLC and/or its Affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com.
 *
 */

#pragma once

#ifndef __MOTR_BTREE_BTREE_H__
#define __MOTR_BTREE_BTREE_H__

#include "lib/types.h"
#include "lib/vec.h"
#include "xcode/xcode_attr.h"
#include "lib/errno.h"
#include "fid/fid.h"  /** struct m0_fid */

/**
 * @defgroup btree
 *
 * @{
 */

struct m0_be_tx;
struct m0_be_tx_credit;

struct m0_btree_type;
struct m0_btree;
struct m0_btree_op;
struct m0_btree_rec;
struct m0_btree_cb;
struct m0_btree_key;
struct m0_btree_idata;
struct m0_btree_cursor;

enum m0_btree_types {
	M0_BT_INVALID = 1,
	M0_BT_BALLOC_GROUP_EXTENTS,
	M0_BT_BALLOC_GROUP_DESC,
	M0_BT_EMAP_EM_MAPPING,
	M0_BT_CAS_CTG,
	M0_BT_COB_NAMESPACE,
	M0_BT_COB_OBJECT_INDEX,
	M0_BT_COB_FILEATTR_BASIC,
	M0_BT_COB_FILEATTR_EA,
	M0_BT_COB_FILEATTR_OMG,
	M0_BT_COB_BYTECOUNT,
	M0_BT_CONFDB,
	M0_BT_UT_KV_OPS,
	M0_BT_NR
};

enum m0_btree_crc_type {
	/**
	 *  No CRC is embedded in Key and/or Value fields of the Record. So no
	 *  verification is needed when the nodes is loaded off the disk.
	 */
	M0_BCT_NO_CRC = 0,

	/**
	 *  CRC is present only in the Value field of the Record. The last word
	 *  of the Value field contains the CRC calculated using function
	 *  m0_has_fnc_fnv1()
	 */
	M0_BCT_USER_ENC_RAW_HASH,

	/**
	 *  CRC is present only in the Value field of the Record. This CRC is
	 *  embedded as a part of structure m0_format_footer which in turn is
	 *  appended to the Value. This footer is generated using the function
	 *  m0_format_footer_generate().
	 */
	M0_BCT_USER_ENC_FORMAT_FOOTER,

	/**
	 * CRC is present in the Value field of the Record. The CRC function is
	 * provided by the Caller. Currently this functionality is not
	 * implemented in the code.
	 */
	M0_BCT_USER_CRC_FUNCTION, /** TBD */

	/**
	 * CRC is internally embedded by btree in Key and/or Value fields of the
	 * Record. No CRC will be provide by the Calller. m0_has_fnc_fnv1() can
	 * be used to calculate CRC.
	 */
	M0_BCT_BTREE_ENC_RAW_HASH,
};

struct m0_btree_type {
	enum m0_btree_types tt_id;
	int ksize;
	int vsize;
};


struct m0_bcookie {
	void     *segaddr;
	uint64_t  n_seq;
};

struct m0_btree_key {
	struct m0_bufvec  k_data;
	struct m0_bcookie k_cookie;
};

struct m0_btree_rec {
	struct m0_btree_key r_key;
	struct m0_bufvec    r_val;
	uint32_t            r_flags;
	uint64_t            r_crc_type;
};

struct m0_btree_cb {
	int (*c_act)(struct m0_btree_cb *cb, struct m0_btree_rec *rec);
	void *c_datum;
};

struct m0_btree_rec_key_op {
	/**
	 * Key comparison function will return -ve, 0 or +ve value depending on
	 * how key0 and key1 compare in key ordering.
	 */
	int (*rko_keycmp)(const void *key0, const void *key1);
};
/**
 * This structure is used to hold the data that is passed to m0_tree_create.
 */
struct m0_btree_idata {
	void                        *addr;
	struct m0_btree             *tree;
	int                          num_bytes;
	const struct m0_btree_type  *bt;
	const struct node_type      *nt;
	enum m0_btree_crc_type       crc_type;
	int                          ks;
	int                          vs;
	struct m0_fid                fid;
};

enum m0_btree_rec_type {
	M0_BRT_VALUE = 1,
	M0_BRT_CHILD = 2,
};

enum m0_btree_opcode {
	M0_BO_CREATE = 1,
	M0_BO_DESTROY,
	M0_BO_GET,
	M0_BO_PUT,
	M0_BO_UPDATE,
	M0_BO_DEL,
	M0_BO_ITER,
	M0_BO_MINKEY,
	M0_BO_MAXKEY,
	M0_BO_TRUNCATE,

	M0_BO_NR
};

enum m0_btree_op_flags {
	BOF_PREV                    = M0_BITS(0),
	BOF_NEXT                    = M0_BITS(1),
	BOF_LOCKALL                 = M0_BITS(2),
	BOF_COOKIE                  = M0_BITS(3),
	BOF_EQUAL                   = M0_BITS(4),
	BOF_SLANT                   = M0_BITS(5),
	BOF_INSERT_IF_NOT_FOUND     = M0_BITS(6),
};

/**
 * These status codes are filled in m0_btree_rec.r_flags to provide the callback
 * routine the status of the operatoin.
 */
enum m0_btree_status_codes {
	M0_BSC_SUCCESS = 0,
	M0_BSC_KEY_EXISTS = EEXIST,
	M0_BSC_KEY_NOT_FOUND = ENOENT,
	M0_BSC_KEY_BTREE_BOUNDARY,
};

enum m0_btree_opflag {
	M0_BOF_UNIQUE = 1 << 0
};

/**
 * Users for triggering LRU list purge.
 */
enum m0_btree_purge_user{
	M0_PU_BTREE,
	M0_PU_EXTERNAL,
};

/** Btree fid type */
M0_EXTERN const struct m0_fid_type m0_btree_fid_type;

#define REC_INIT(p_rec, pp_key, p_ksz, pp_val, p_vsz)                          \
	({                                                                     \
		M0_CASSERT(M0_HAS_TYPE((p_rec), struct m0_btree_rec *));       \
									       \
		(p_rec)->r_key.k_data = M0_BUFVEC_INIT_BUF((pp_key), (p_ksz)); \
		(p_rec)->r_val        = M0_BUFVEC_INIT_BUF((pp_val), (p_vsz)); \
	})

#define REC_INIT_WITH_CRC(p_rec, pp_key, p_ksz, pp_val, p_vsz, crc_type)       \
	({                                                                     \
		REC_INIT(p_rec, pp_key, p_ksz, pp_val, p_vsz);                 \
		(p_rec)->r_crc_type   = crc_type;                              \
	})

#define COPY_RECORD(p_tgt, p_src)                                              \
	({                                                                     \
		struct m0_btree_rec *__tgt_rec = (p_tgt);                      \
		struct m0_btree_rec *__src_rec = (p_src);                      \
									       \
		M0_CASSERT(M0_HAS_TYPE((p_tgt), struct m0_btree_rec *));       \
		M0_CASSERT(M0_HAS_TYPE((p_src), struct m0_btree_rec *));       \
		m0_bufvec_copy(&__tgt_rec->r_key.k_data,                       \
			       &__src_rec ->r_key.k_data,                      \
			       m0_vec_count(&__src_rec ->r_key.k_data.ov_vec));\
		m0_bufvec_copy(&__tgt_rec->r_val,                              \
			       &__src_rec->r_val,                              \
			       m0_vec_count(&__src_rec ->r_val.ov_vec));       \
	})

/**
 * Btree functions related to credit management for tree operations
 */

/**
 * Calculates the credit needed to create tree with @nr nodes and adds this
 * credit to @accum.
 *
 * @param bt points to the structure which tells the tree type for whom the
 *        credit needs to be calculated.
 * @param accum contains the accumulated credit count.
 * @param nr is the multiplier which gets the total credits for that many nodes
 *        of the btree.
 */
M0_INTERNAL void m0_btree_create_credit(const struct m0_btree_type *bt,
					struct m0_be_tx_credit *accum,
					m0_bcount_t nr);

/**
 * Calculates the credit needed to destroy tree with @nr nodes and adds this
 * credit to @accum.
 * Either tree or bt should be valid (non-NULL). If both are valid then tree is
 * used to get the credit count.
 *
 * @param tree points to the tree for whom the credits need to be calculated.
 * @param bt points to the structure which tells the tree type for whom the
 *        credit needs to be calculated.
 * @param accum contains the accumulated credit count.
 * @param nr is the multiplier which gets the total credits for that many nodes
 *        of the btree.
 */
M0_INTERNAL void m0_btree_destroy_credit(struct m0_btree *tree,
					 const struct m0_btree_type *bt,
					 struct m0_be_tx_credit *accum,
					 m0_bcount_t nr);

/**
 * Calculates the credits for maximum nodes that can be deleted at a time from
 * btree. To be on the safer side, the calculation takes half of the
 * maximum transaction credit as the maximum transaction limit.
 */
M0_INTERNAL void m0_btree_truncate_credit(struct m0_be_tx        *tx,
					  struct m0_btree        *tree,
					  struct m0_be_tx_credit *accum,
					  m0_bcount_t            *limit);
/**
 * Btree functions related to tree management
 */


/**
 * Opens the tree and returns the pointer to m0_btree which is used for
 * subsequent operations related to this btree.
 *
 * @param addr is the address of exsiting root node in BE segment.
 * @param nob is the size of the root node in BE segment.
 * @param out points to the User allocated space which can be used in subsequent
 *        btree operations.
 * @param seg points to the BE segment which hosts the nodes of the tree.
 * @param bop is consumed by the m0_btree_open for its operation.
 * @param keycmp contains pointer to the key comparison function which is
 *        provided by the caller and used by the btree routines when working on
 *        this btree.
 *
 * @return 0 if successful.
 */
M0_INTERNAL int  m0_btree_open(void *addr, int nob, struct m0_btree *out,
			       struct m0_be_seg *seg, struct m0_btree_op *bop,
			       struct m0_btree_rec_key_op *keycmp);

/**
 * Closes the opened or created tree represented by arbor. Once the close
 * completes no further actions should be triggered for this btree until the
 * btree is opened again by calling m0_btree_open()
 *
 * If some of the nodes for this btree are still active in different threads or
 * FOMs then this function waits till all the active nodes of this btree have
 * been 'PUT' or destroyed.
 *
 * @param arbor is the btree which needs to be closed.
 */
M0_INTERNAL void m0_btree_close(struct m0_btree *arbor, struct m0_btree_op *bop);

/**
 * Creates a new btree with the root node created at the address passed as the
 * parameter. The space of size nob for this root node is assumed to be
 * allocated, in the BE segment, by the caller. This function initializes and
 * uses this space for holding the root node of this newly created tree. The
 * routine return the pointer to m0_btree which is used for subsequent
 * operations related to this btree.
 * m0_btree_create(), if successful, returns the tree handle which can be used
 * for subsequent tree operations without having to call m0_tree_open().
 *
 * @param addr is the address of exsiting root node in BE segment.
 * @param nob is the size of the root node in BE segment.
 * @param bt provides more information about the btree to be created.
 * @param bop is consumed by the m0_btree_create for its operation. It contains
 *        the field bo_arbor which holds the tree pointer to be used by the
 *        caller after the call completes.
 * @param tree points to the User allocated space which create will intitialize
 *        and subsequent btree routines will use it as handle to btree.
 * @param seg points to the BE segment which will host the nodes of the tree.
 * @param fid unique fid of the tree.
 * @param tx pointer to the transaction struture to capture BE segment changes.
 * @param keycmp contains pointer to the key comparison function which is
 *        provided by the caller and used by the btree routines when working on
 *        this btree.
 */
M0_INTERNAL void m0_btree_create(void *addr, int nob,
				 const struct m0_btree_type *bt,
				 enum m0_btree_crc_type crc_type,
				 struct m0_btree_op *bop, struct m0_btree *tree,
				 struct m0_be_seg *seg,
				 const struct m0_fid *fid, struct m0_be_tx *tx,
				 struct m0_btree_rec_key_op *keycmp);

/**
 * Destroys the opened or created tree represented by arbor. Once the destroy
 * completes no further actions should be triggered for this btree as this tree
 * should be assumed to be deleted.
 * This routine expects all the nodes of this tree to have been deleted and no
 * records to be present in this btree.
 *
 * @param arbor is the btree which needs to be closed.
 * @param bop is consumed by the m0_btree_destroy for its operation.
 * @param tx pointer to the transaction structure to capture BE segment changes.
 */
M0_INTERNAL void m0_btree_destroy(struct m0_btree *arbor,
				  struct m0_btree_op *bop, struct m0_be_tx *tx);

/**
 * Searches for the key/slant key provided as the search key. The callback
 * routine is called when the search yields the key/slant key and the location
 * of this key/slant key is passed to the callback.
 * The callback is NOT supposed to modify this record since these changes will
 * not get captured in any transaction and hence m0_btree_put() should be called
 * by the caller instead.
 *
 * @param arbor is the pointer to btree.
 * @param key   is the Key to be searched in the btree.
 * @param cb    Callback routine to be called on search success.
 * @param flags Operation specific flags (cookie, slant etc.).
 * @param bop   Btree operation related parameters.
 */
M0_INTERNAL void m0_btree_get(struct m0_btree *arbor,
			      const struct m0_btree_key *key,
			      const struct m0_btree_cb *cb, uint64_t flags,
			      struct m0_btree_op *bop);

/**
 * Inserts the record in the tree. The callback is called with the location
 * where the new key and value should be inserted in the tree.
 *
 * @param arbor is the pointer to btree.
 * @param rec   represents the record which needs to get inserted. Note that,
 *              user may or may not provide valid value but record should be
 *              provided with valid key, key size and value size as this
 *              information is needed for correct operation.
 * @param cb    routine to be called to PUT the record.
 * @param bop   Btree operation related parameters.
 * @param tx    represents the transaction of which the current operation is
 *              part of.
 */
M0_INTERNAL void m0_btree_put(struct m0_btree *arbor,
			      const struct m0_btree_rec *rec,
			      const struct m0_btree_cb *cb,
			      struct m0_btree_op *bop, struct m0_be_tx *tx);

/**
 * Updates or inserts the record in the tree depending on @param flags. The
 * callback is called with the location where the updated value or new record
 * should be written in the tree.
 *
 * @param arbor is the pointer to btree.
 * @param rec   represents the record which needs to be updated. Note that,
 *              user may or may not provide valid value but record should be
 *              provided with valid key, key size and value size as this
 *              information is needed for correct operation.
 * @param cb    routine to be called to PUT the record.
 * @param flags If flag is set to BOF_PUT_IF_NOT_EXIST and key is not present,
 *              the given record will be added to the btree.
 * @param bop   Btree operation related parameters.
 * @param tx    represents the transaction of which the current operation is
 *              part of.
 */
M0_INTERNAL void m0_btree_update(struct m0_btree *arbor,
				 const struct m0_btree_rec *rec,
				 const struct m0_btree_cb *cb, uint64_t flags,
				 struct m0_btree_op *bop, struct m0_be_tx *tx);

/**
 * Deletes the record in the tree. The callback is called with the location
 * where the original key and value should are present in the tree.
 *
 * @param arbor is the pointer to btree.
 * @param key   points to the Key whose record is to be deleted from the tree.
 * @param bop   Btree operation related parameters.
 * @param tx    represents the transaction of which the current operation is
 *              part of.
 */
M0_INTERNAL void m0_btree_del(struct m0_btree *arbor,
			      const struct m0_btree_key *key,
			      const struct m0_btree_cb *cb,
			      struct m0_btree_op *bop, struct m0_be_tx *tx);

/**
 * Iterates through the tree and finds next/previous key from the given search
 * key based on the flag. The callback routine is provided with the record of
 * the next/previous Key which was found in the tree.
 *
 * @param arbor Btree parameteres.
 * @param key   Key to be searched in the btree.
 * @param cb    Callback routine to return operation output.
 * @param flags Operation specific flags (cookie, slant, prev, next etc.).
 * @param bop   Btree operation related parameters.
 */
M0_INTERNAL void m0_btree_iter(struct m0_btree *arbor,
			       const struct m0_btree_key *key,
			       const struct m0_btree_cb *cb, uint64_t flags,
			       struct m0_btree_op *bop);

/**
 * Returns the records corresponding to minimum key of the btree.
 *
 * @param arbor Btree parameteres.
 * @param cb    Callback routine to return operation output.
 * @param flags Operation specific flags (cookie, lockall etc.).
 * @param bop   Btree operation related parameters.
 */
M0_INTERNAL void m0_btree_minkey(struct m0_btree *arbor,
				 const struct m0_btree_cb *cb, uint64_t flags,
				 struct m0_btree_op *bop);

/**
 * Returns the records corresponding to maximum key of the btree.
 *
 * @param arbor Btree parameteres.
 * @param cb    Callback routine to return operation output.
 * @param flags Operation specific flags (cookie, lockall etc.).
 * @param bop   Btree operation related parameters.
 */
M0_INTERNAL void m0_btree_maxkey(struct m0_btree *arbor,
				 const struct m0_btree_cb *cb, uint64_t flags,
				 struct m0_btree_op *bop);

/**
 * Deletes all the nodes except root node. It deletes all the records from the
 * root node and keeps root node empty. This routine may be called multiple
 * times to fit into transaction.
 *
 * @param arbor Btree parameteres.
 * @param limit Maximum number of nodes to be deleted.
 * @param tx    Transaction of which the current operation is part of.
 * @param bop   Btree operation related parameters.
 */
M0_INTERNAL void m0_btree_truncate(struct m0_btree *arbor, m0_bcount_t limit,
				   struct m0_be_tx *tx,
				   struct m0_btree_op *bop);

/**
 * Initialises cursor and its internal structures.
 *
 * @param it    is pointer to cursor structure allocated by the caller.
 * @param arbor is the pointer to btree.
 */
M0_INTERNAL void m0_btree_cursor_init(struct m0_btree_cursor *it,
				      struct m0_btree        *arbor);

/**
 * Finalizes cursor.
 *
 * @param it  is pointer to cursor structure allocated by the caller.
 */
M0_INTERNAL void m0_btree_cursor_fini(struct m0_btree_cursor *it);

/**
 * Fills cursor internal buffers with current key and value obtained from the
 * tree.
 *
 * @param it    is pointer to cursor structure.
 * @param key   is the Key to be searched in the btree.
 * @param slant is TRUE if slant-search is to be executed.
 *
 * @return 0 if successful.
 */
M0_INTERNAL int m0_btree_cursor_get(struct m0_btree_cursor    *it,
				    const struct m0_btree_key *key,
				    bool                       slant);

/**
 * Fills cursor internal buffers with key and value obtained from the
 * next position in tree. The operation is unprotected from concurrent btree
 * updates and user should protect it with external lock.
 *
 * @param it  is pointer to cursor structure.
 */
M0_INTERNAL int m0_btree_cursor_next(struct m0_btree_cursor *it);

/**
 * Fills cursor internal buffers with prev key and value obtained from the
 * tree.
 *
 * @param it  is pointer to cursor structure.
 */
M0_INTERNAL int m0_btree_cursor_prev(struct m0_btree_cursor *it);

/**
 * Moves cursor to the first key in the btree.
 *
 * @param it  is pointer to cursor structure.
 */
M0_INTERNAL int m0_btree_cursor_first(struct m0_btree_cursor *it);

/**
 * Moves cursor to the last key in the btree.
 *
 * @param it  is pointer to cursor structure.
 */
M0_INTERNAL int m0_btree_cursor_last(struct m0_btree_cursor *it);

/**
 * Releases cursor values.
 *
 * @param it  is pointer to cursor structure.
 */
M0_INTERNAL void m0_btree_cursor_put(struct m0_btree_cursor *it);

/**
 * Sets key and value buffers to point on internal structures of cursor
 * representing current key and value, cursor is placed on.
 *
 * Any of @key and @val pointers can be NULL, but not both.
 *
 * @param it  is pointer to cursor structure.
 * @param key will point to the cursor pointed Key by this routine.
 * @param val will point to the cursor pointed Value by this routine.
 */
M0_INTERNAL void m0_btree_cursor_kv_get(struct m0_btree_cursor *it,
					struct m0_buf          *key,
					struct m0_buf          *val);

/**
 * Determines if tree contains zero record.
 */
M0_INTERNAL bool m0_btree_is_empty(struct m0_btree *btree);

void m0_btree_op_init(struct m0_btree_op *bop, enum m0_btree_opcode *opc,
		      struct m0_btree *arbor,
		      struct m0_btree_key *key, const struct m0_btree_cb *cb,
		      uint64_t flags, struct m0_be_tx *tx);
void m0_btree_op_fini(struct m0_btree_op *bop);

void m0_btree_op_credit(const struct m0_btree_op *bt,
			struct m0_be_tx_credit *cr);

/**
 * Calculates credits required to perform 'nr' Put KV operations. The calculated
 * credits will be added to the existing value in accum.
 * The routine assumes that each of the Keys and Values to be Put will be of
 * size ksize and vsize respectively.
 *
 * @param tree  is the pointer to btree.
 * @param nr    is the number of records for which the credit count is desired.
 * @param ksize is the size of the Key which will be added.
 * @param vsize is the size of the Value which will be added.
 * @param accum will contain the calculated credits.
 */
M0_INTERNAL void m0_btree_put_credit(const struct m0_btree  *tree,
				     m0_bcount_t             nr,
				     m0_bcount_t             ksize,
				     m0_bcount_t             vsize,
				     struct m0_be_tx_credit *accum);

/**
 * Calculates credits required to perform 'nr' Put KV operations. The calculated
 * credits will be added to the existing value in accum.
 * The routine assumes that each of the Keys and Values to be Put will be of
 * size ksize and vsize respectively.
 * This routine should be used when m0_btree is not available but the type of
 * the node used in the btree is known.
 *
 * Note: Avoid using this routine unless the m0_btree is not available since
 * this routine is slower when compared to m0_btree_put_credit().
 *
 * @param type      points to the type of btree.
 * @param rnode_nob is the byte count of root node of this btree.
 * @param nr        is the number of records for which the credit count is
 *                  desired.
 * @param ksize     is the size of the Key which will be added.
 * @param vsize     is the size of the Value which will be added.
 * @param accum     will contain the calculated credits.
 */
M0_INTERNAL void m0_btree_put_credit2(const struct m0_btree_type *type,
				      int                         rnode_nob,
				      m0_bcount_t                 nr,
				      m0_bcount_t                 ksize,
				      m0_bcount_t                 vsize,
				      struct m0_be_tx_credit     *accum);

/**
 * Calculates credits required to perform 'nr' Update KV operations. The
 * calculated credits will be added to the existing value in accum.
 * The routine assumes that each of the Keys and Values to be Updated will be of
 * size ksize and vsize respectively.
 *
 * @param tree  is the pointer to btree.
 * @param nr    is the number of records for which the credit count is desired.
 * @param ksize is the size of the Key which will be added.
 * @param vsize is the size of the Value which will be added.
 * @param accum will contain the calculated credits.
 */
M0_INTERNAL void m0_btree_update_credit(const struct m0_btree  *tree,
					m0_bcount_t             nr,
					m0_bcount_t             ksize,
					m0_bcount_t             vsize,
					struct m0_be_tx_credit *accum);

/**
 * Calculates credits required to perform 'nr' Update KV operations. The
 * calculated credits will be added to the existing value in accum.
 * The routine assumes that each of the Keys and Values to be Updated will be of
 * size ksize and vsize respectively.
 * This routine should be used when m0_btree is not available but the type of
 * the node used in the btree is known.
 *
 * Note: Avoid using this routine unless the m0_btree is not available since
 * this routine is slower when compared to m0_btree_update_credit().
 *
 * @param type      points to the type of btree.
 * @param rnode_nob is the byte count of root node of this btree.
 * @param nr        is the number of records for which the credit count is
 *                  desired.
 * @param ksize     is the size of the Key which will be added.
 * @param vsize     is the size of the Value which will be added.
 * @param accum     will contain the calculated credits.
 */
M0_INTERNAL void m0_btree_update_credit2(const struct m0_btree_type *type,
					 int                         rnode_nob,
					 m0_bcount_t                 nr,
					 m0_bcount_t                 ksize,
					 m0_bcount_t                 vsize,
					 struct m0_be_tx_credit     *accum);

/**
 * Calculates credits required to perform 'nr' Delete KV operations. The
 * calculated credits will be added to the existing value in accum.
 * The routine assumes that each of the Keys and Values to be Deleted will be of
 * size ksize and vsize respectively.
 *
 * @param tree  is the pointer to btree.
 * @param nr    is the number of records for which the credit count is desired.
 * @param ksize is the size of the Key which will be added.
 * @param vsize is the size of the Value which will be added.
 * @param accum will contain the calculated credits.
 */
M0_INTERNAL void m0_btree_del_credit(const struct m0_btree  *tree,
				     m0_bcount_t             nr,
				     m0_bcount_t             ksize,
				     m0_bcount_t             vsize,
				     struct m0_be_tx_credit *accum);

/**
 * Calculates credits required to perform 'nr' Delete KV operations. The
 * calculated credits will be added to the existing value in accum.
 * The routine assumes that each of the Keys and Values to be Deleted will be of
 * size ksize and vsize respectively.
 * This routine should be used when m0_btree is not available but the type of
 * the node used in the btree is known.
 *
 * Note: Avoid using this routine unless the m0_btree is not available since
 * this routine is slower when compared to m0_btree_del_credit().
 *
 * @param type      points to the type of btree.
 * @param rnode_nob is the byte count of root node of this btree.
 * @param nr        is the number of records for which the credit count is
 *                  desired.
 * @param ksize     is the size of the Key which will be added.
 * @param vsize     is the size of the Value which will be added.
 * @param accum     will contain the calculated credits.
 */
M0_INTERNAL void m0_btree_del_credit2(const struct m0_btree_type *type,
				      int                         rnode_nob,
				      m0_bcount_t                 nr,
				      m0_bcount_t                 ksize,
				      m0_bcount_t                 vsize,
				      struct m0_be_tx_credit     *accum);

#include "be/btree_internal.h"

M0_INTERNAL int     m0_btree_mod_init(void);
M0_INTERNAL void    m0_btree_mod_fini(void);
M0_INTERNAL void    m0_btree_glob_init(void);
M0_INTERNAL void    m0_btree_glob_fini(void);
M0_INTERNAL int64_t m0_btree_lrulist_purge(int64_t size, int64_t num_nodes);
M0_INTERNAL int64_t m0_btree_lrulist_purge_check(enum m0_btree_purge_user user,
						 int64_t size);
M0_INTERNAL void    m0_btree_lrulist_set_lru_config(int64_t slow_lru_mem_release,
						    int64_t wm_low,
						    int64_t wm_target,
						    int64_t wm_high);

#define M0_BTREE_OP_SYNC_WITH_RC(bop, action)                           \
	({                                                              \
		struct m0_btree_op *__bopp = (bop);                     \
		int32_t             __op_rc;                            \
									\
		m0_sm_group_init(&__bopp->bo_sm_group);                 \
		m0_sm_group_lock(&__bopp->bo_sm_group);                 \
		m0_sm_op_exec_init(&__bopp->bo_op_exec);                \
									\
		action;                                                 \
		m0_sm_op_tick(&__bopp->bo_op);                          \
		__op_rc = __bopp->bo_op.o_sm.sm_rc;                     \
		m0_sm_op_fini(&__bopp->bo_op);                          \
									\
		m0_sm_op_exec_fini(&__bopp->bo_op_exec);                \
		m0_sm_group_unlock(&__bopp->bo_sm_group);               \
		m0_sm_group_fini(&__bopp->bo_sm_group);                 \
		__op_rc;                                                \
	})



/** @} end of btree group */
#endif /* __MOTR_BTREE_BTREE_H__ */

/*
 *  Local variables:
 *  c-indentation-style: "K&R"
 *  c-basic-offset: 8
 *  tab-width: 8
 *  fill-column: 80
 *  scroll-step: 1
 *  End:
 */
/*
 * vim: tabstop=8 shiftwidth=8 noexpandtab textwidth=80 nowrap
 */
