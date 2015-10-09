# include "rrdb.server.impl.h"
# include <algorithm>
# include <dsn/cpp/utils.h>
# include <rocksdb/utilities/checkpoint.h>

# ifdef __TITLE__
# undef __TITLE__
# endif
#define __TITLE__ "rrdb.server.impl"

namespace dsn {
    namespace apps {
        
        static std::string chkpt_get_dir_name(rocksdb::SequenceNumber last_seq)
        {
            char buffer[256];
            sprintf(buffer, "checkpoint.%lld", last_seq);
            return std::string(buffer);
        }

        static bool chkpt_init_from_dir(const char* name, rocksdb::SequenceNumber& last_seq)
        {
            return 1 == sscanf(name, "checkpoint.%lld", &last_seq);
        }

        rrdb_service_impl::rrdb_service_impl(::dsn::replication::replica* replica)
            : rrdb_service(replica)
        {
            _is_open = false;

            // disable write ahead logging as replication handles logging instead now
            _wt_opts.disableWAL = false;
        }

        ::dsn::replication::decree rrdb_service_impl::parse_for_checkpoints()
        {
            std::vector<std::string> dirs;
            utils::filesystem::get_subdirectories(data_dir(), dirs, false);

            _checkpoints.clear();
            for (auto& d : dirs)
            {
                rocksdb::SequenceNumber ch;
                std::string d1 = d;
                d1 = d1.substr(data_dir().length() + 1);
                if (chkpt_init_from_dir(d1.c_str(), ch))
                {
                    _checkpoints.push_back(ch);
                }
            }

            std::sort(_checkpoints.begin(), _checkpoints.end());

            return _checkpoints.size() > 0 ? *_checkpoints.rbegin() : 0;
        }

        void rrdb_service_impl::on_empty_write()
        {
            dassert(_is_open, "rrdb service %s is not ready", data_dir().c_str());
            
            update_request update;
            update.key = blob("empty-0xdeadbeef", 0, 16);
            update.value = blob("empty", 0, 5);

            ::dsn::rpc_replier<int> reply(nullptr);

            on_put(update, reply);
        }

        void rrdb_service_impl::on_put(const update_request& update, ::dsn::rpc_replier<int>& reply)
        {
            dassert(_is_open, "rrdb service %s is not ready", data_dir().c_str());
            
            rocksdb::WriteOptions opts = _wt_opts;
            opts.given_sequence_number = last_committed_decree() + 1;

            rocksdb::Slice skey(update.key.data(), update.key.length());
            rocksdb::Slice svalue(update.value.data(), update.value.length());
            rocksdb::Status status = _db->Put(opts, skey, svalue);
            dassert(status.ok(), status.ToString().c_str());
            dassert(_db->GetLatestSequenceNumber() == last_committed_decree() + 1,
                "mismatched sequencenumber %lld vs %lld",
                _db->GetLatestSequenceNumber(),
                last_committed_decree() + 1
                );
            reply(status.code());

            ++_last_committed_decree;
        }

        void rrdb_service_impl::on_remove(const ::dsn::blob& key, ::dsn::rpc_replier<int>& reply)
        {
            dassert(_is_open, "rrdb service %s is not ready", data_dir().c_str());
            
            rocksdb::WriteOptions opts = _wt_opts;
            opts.given_sequence_number = last_committed_decree() + 1;

            rocksdb::Slice skey(key.data(), key.length());
            rocksdb::Status status = _db->Delete(opts, skey);
            dassert(status.ok(), status.ToString().c_str());
            dassert(_db->GetLatestSequenceNumber() == last_committed_decree() + 1, 
                "mismatched sequencenumber %lld vs %lld",
                _db->GetLatestSequenceNumber(),
                last_committed_decree() + 1
                );
            reply(status.code());

            ++_last_committed_decree;
        }

        void rrdb_service_impl::on_merge(const update_request& update, ::dsn::rpc_replier<int>& reply)
        {
            dassert(_is_open, "rrdb service %s is not ready", data_dir().c_str());
            
            rocksdb::WriteOptions opts = _wt_opts;
            opts.given_sequence_number = last_committed_decree() + 1;

            rocksdb::Slice skey(update.key.data(), update.key.length());
            rocksdb::Slice svalue(update.value.data(), update.value.length());
            rocksdb::Status status = _db->Merge(opts, skey, svalue);
            dassert(status.ok(), status.ToString().c_str());
            dassert(_db->GetLatestSequenceNumber() == last_committed_decree() + 1,
                "mismatched sequencenumber %lld vs %lld",
                _db->GetLatestSequenceNumber(),
                last_committed_decree() + 1
                );
            reply(status.code());

            ++_last_committed_decree;
        }

        void rrdb_service_impl::on_get(const ::dsn::blob& key, ::dsn::rpc_replier<read_response>& reply)
        {
            dassert(_is_open, "rrdb service %s is not ready", data_dir().c_str());

            read_response resp;
            rocksdb::Slice skey(key.data(), key.length());
            rocksdb::Status status = _db->Get(_rd_opts, skey, &resp.value);
            resp.error = status.code();

            reply(resp);
        }

        int  rrdb_service_impl::open(bool create_new)
        {
            dassert(!_is_open, "rrdb service %s is already opened", data_dir().c_str());

            rocksdb::Options opts;
            opts.create_if_missing = create_new;
            opts.error_if_exists = create_new;
            opts.write_buffer_size = 40 * 1024; // 40 K for testing now

            if (create_new)
            {
                auto& dir = data_dir();
                dsn::utils::filesystem::remove_path(dir);
                dsn::utils::filesystem::create_directory(dir);
            }

            auto status = rocksdb::DB::Open(opts, data_dir() + "/rdb", &_db);
            if (status.ok())
            {
                _is_open = true;
                _last_committed_decree = _db->GetLatestSequenceNumber();
                _last_durable_decree = parse_for_checkpoints();

                ddebug("open app %s: last_committed/durable decree = <%llu, %llu>",
                    data_dir().c_str(), last_committed_decree(), last_durable_decree());
            }
            else
            {
                derror("open app %s failed, err = %s",
                    data_dir().c_str(),
                    status.ToString().c_str()
                    );
            }
            return status.code();
        }

        int  rrdb_service_impl::close(bool clear_state)
        {
            if (!_is_open)
            {
                dassert(_db == nullptr, "");
                dassert(!clear_state, ""); // should not be here if do clear
                return 0;
            }

            _is_open = false;
            delete _db;
            _db = nullptr;

            if (clear_state)
            {
                if (!dsn::utils::filesystem::remove_path(data_dir()))
                {
                    dassert(false, "Fail to delete directory %s.", data_dir().c_str());
                }
            }

            return 0;
        }

        int  rrdb_service_impl::flush(bool wait)
        {
            dassert(_is_open, "rrdb service %s is not ready", data_dir().c_str());

            if (_last_durable_decree == _last_committed_decree)
                return 0;

            rocksdb::Checkpoint* chkpt = nullptr;
            auto status = rocksdb::Checkpoint::Create(_db, &chkpt);
            if (!status.ok())
            {
                return status.code();
            }

            rocksdb::SequenceNumber ch = last_committed_decree();
            auto dir = chkpt_get_dir_name(ch);

            auto chkpt_dir = utils::filesystem::path_combine(data_dir(), dir);
            
            status = chkpt->CreateCheckpoint(chkpt_dir);
            if (status.ok())
            {
                _last_durable_decree = last_committed_decree();
                _checkpoints.push_back(ch);
            }

            delete chkpt;
            return status.code();
        }

        void rrdb_service_impl::prepare_learning_request(/*out*/ blob& learn_req)
        {
            // nothing to do
        }

        int  rrdb_service_impl::get_learn_state(
            ::dsn::replication::decree start, 
            const blob& learn_req, 
            /*out*/ ::dsn::replication::learn_state& state)
        {
            // simply copy all files

            dassert(_is_open, "rrdb service %s is not ready", data_dir().c_str());

            if (_checkpoints.size() > 0)
            {
                rocksdb::SequenceNumber ch = *_checkpoints.rbegin();
                auto dir = chkpt_get_dir_name(ch);

                auto chkpt_dir = utils::filesystem::path_combine(data_dir(), dir);
                auto err = utils::filesystem::get_subfiles(chkpt_dir, state.files, true);
                dassert(err, "list files in chkpoint dir %s failed", chkpt_dir.c_str());
            }
            
            return 0;
        }

        int  rrdb_service_impl::apply_learn_state(::dsn::replication::learn_state& state)
        {
            int err = 0;

            if (_is_open)
            {
                // clear db
                err = close(true);
                if (err != 0)
                {
                    derror("clear db %s failed, err = %d", data_dir().c_str(), err);
                    return err;
                }
            }
            else
            {
                ::dsn::utils::filesystem::remove_path(data_dir());
            }

            // create data dir first
            ::dsn::utils::filesystem::create_directory(data_dir());

            // move learned files from learn_dir to data_dir
            std::string learn_dir = ::dsn::utils::filesystem::remove_file_name(*state.files.rbegin());
            std::string new_dir = ::dsn::utils::filesystem::path_combine(
                data_dir(),
                "rdb"
                );

            if (!::dsn::utils::filesystem::rename_path(learn_dir, new_dir))
            {
                derror("rename %s to %s failed", learn_dir.c_str(), new_dir.c_str());
                return ERR_FILE_OPERATION_FAILED;
            }

            /*for (auto &old_p : state.files)
            {
            }*/

            // reopen the db with the new checkpoint files
            if (state.files.size() > 0)
                err = open(false);
            else
                err = open(true);

            if (err != 0)
            {
                derror("open db %s failed, err = %d", data_dir().c_str(), err);
                return err;
            }
            else
            {
                flush(true);
            }

            return 0;
        }
                
        ::dsn::replication::decree rrdb_service_impl::last_durable_decree() const
        {
            return _last_durable_decree.load();
        }
    }
}
