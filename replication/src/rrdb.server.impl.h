# pragma once

# include "rrdb.server.h"
# include <rocksdb/db.h>
# include <vector>

namespace dsn {
    namespace apps {
        class rrdb_service_impl : public rrdb_service
        {
        public:
            rrdb_service_impl(::dsn::replication::replica* replica);

            virtual void on_put(const update_request& update, ::dsn::rpc_replier<int>& reply);
            virtual void on_remove(const ::dsn::blob& key, ::dsn::rpc_replier<int>& reply);
            virtual void on_merge(const update_request& update, ::dsn::rpc_replier<int>& reply);
            virtual void on_get(const ::dsn::blob& key, ::dsn::rpc_replier<read_response>& reply);

            virtual int  open(bool create_new);
            virtual int  close(bool clear_state);
            virtual int  checkpoint();
            virtual int  checkpoint_async();
            
            virtual int  get_checkpoint(::dsn::replication::decree start,
                    const blob& learn_req, /*out*/ ::dsn::replication::learn_state& state);
            virtual int  apply_checkpoint(::dsn::replication::learn_state& state, ::dsn::replication::chkpt_apply_mode mode);
            virtual ::dsn::replication::decree last_durable_decree() const;

        private:
            struct checkpoint_info
            {
                ::dsn::replication::decree d;
                rocksdb::SequenceNumber    seq;
            };

            checkpoint_info parse_for_checkpoints();
            void gc_checkpoints();
            void write_batch();

        private:
            rocksdb::DB           *_db;
            rocksdb::WriteBatch   _batch;
            std::vector<rpc_replier<int>> _batch_repliers;
            rocksdb::WriteOptions _wt_opts;
            rocksdb::ReadOptions  _rd_opts;

            std::atomic<bool>     _is_open;            
            const int             _max_checkpoint_count;
                        
            rocksdb::SequenceNumber      _last_seq;
            rocksdb::SequenceNumber      _last_durable_seq; // valid only when _is_catchup is true
            bool                         _is_catchup;       // whether the db is in catch up mode
            std::vector<checkpoint_info> _checkpoints;
            ::dsn::utils::ex_lock_nr     _checkpoints_lock;
        };

        // --------- inline implementations -----------------
    }
}
