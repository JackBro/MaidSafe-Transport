/* Copyright (c) 2009 maidsafe.net limited
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
    * Neither the name of the maidsafe.net limited nor the names of its
    contributors may be used to endorse or promote products derived from this
    software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef MAIDSAFE_KADEMLIA_DATASTORE_H_
#define MAIDSAFE_KADEMLIA_DATASTORE_H_

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/cstdint.hpp>
#include <boost/thread/mutex.hpp>
#include <string>
#include <vector>
#include <set>
#include <utility>

namespace kademlia {
// This class implements physical storage (for data published and fetched via
// the RPCs) for the Kademlia DHT. Boost::multiindex are used

enum DeleteStatus {
  kNotDeleted,
  kMarkedForDeletion,
  kDeleted
};

struct RefreshValue {
  std::string key, value;
  boost::int32_t ttl;
  DeleteStatus delete_status;
  RefreshValue(const std::string &key, const std::string &value, const
               boost::int32_t &ttl)
      : key(key), value(value), ttl(ttl), delete_status(kNotDeleted) {}
  RefreshValue(const std::string &key, const std::string &value, const
               DeleteStatus &delete_status)
      : key(key), value(value), ttl(0), delete_status(delete_status) {}
};

struct KeyValueTuple {
  std::string key, value, serialized_delete_request;
  boost::uint32_t last_refresh_time, expire_time;
  boost::int32_t ttl;
  bool hashable;
  DeleteStatus delete_status;
  KeyValueTuple(const std::string &key, const std::string &value,
                const boost::uint32_t &last_refresh_time,
                const boost::uint32_t &expire_time_value,
                const boost::int32_t &ttl, const bool &hashable)
      : key(key), value(value), serialized_delete_request(),
        last_refresh_time(last_refresh_time), expire_time(expire_time_value),
        ttl(ttl), hashable(hashable), delete_status(kNotDeleted) {
    if (ttl < 0)
      expire_time = 0;
  }
  KeyValueTuple(const std::string &key, const std::string &value,
                const boost::uint32_t &last_refresh_time)
      : key(key), value(value), serialized_delete_request(),
        last_refresh_time(last_refresh_time), expire_time(0), ttl(0),
        hashable(true), delete_status(kNotDeleted) {}
};

struct TagKey {};
struct TagLastRefreshTime {};
struct TagExpireTime {};

typedef boost::multi_index::multi_index_container<
  KeyValueTuple,
  boost::multi_index::indexed_by<
    boost::multi_index::ordered_unique<
      boost::multi_index::tag<TagKey>,
      boost::multi_index::composite_key<
        KeyValueTuple,
        BOOST_MULTI_INDEX_MEMBER(KeyValueTuple, std::string, key),
        BOOST_MULTI_INDEX_MEMBER(KeyValueTuple, std::string, value)
      >
    >,
    boost::multi_index::ordered_non_unique<
      boost::multi_index::tag<TagLastRefreshTime>,
      BOOST_MULTI_INDEX_MEMBER(KeyValueTuple, boost::uint32_t,
                               last_refresh_time)
    >,
    boost::multi_index::ordered_non_unique<
      boost::multi_index::tag<TagExpireTime>,
      BOOST_MULTI_INDEX_MEMBER(KeyValueTuple, boost::uint32_t,
                               expire_time)
    >
  >
> KeyValueIndex;

class DataStore {
 public:
  // t_refresh = refresh time of key/value pair in seconds
  explicit DataStore(const boost::uint32_t &refresh_time);
  ~DataStore();
  bool Keys(std::set<std::string> *keys);
  bool HasItem(const std::string &key);
  // time_to_live is in seconds.
  bool StoreItem(const std::string &key, const std::string &value,
                 const boost::int32_t &ttl, const bool &hashable);
  bool LoadItem(const std::string &key, std::vector<std::string> *values);
  bool DeleteKey(const std::string &key);
  bool DeleteItem(const std::string &key, const std::string &value);
  void DeleteExpiredValues();

  boost::uint32_t LastRefreshTime(const std::string &key,
                                  const std::string &value);
  boost::uint32_t ExpireTime(const std::string &key, const std::string &value);
  std::vector<RefreshValue> ValuesToRefresh();
  boost::int32_t TimeToLive(const std::string &key, const std::string &value);
  void Clear();
  std::vector< std::pair<std::string, bool> > LoadKeyAppendableAttr(
      const std::string &key);
  bool RefreshItem(const std::string &key, const std::string &value,
                   std::string *stored_delete_request);
  // If key, value pair does not exist, then it returns false
  bool MarkForDeletion(const std::string &key, const std::string &value,
                       const std::string &serialized_delete_request);
  // If key, value pair does not exist or its status is not MARKED_FOR_DELETION,
  // then it returns false
  bool MarkAsDeleted(const std::string &key, const std::string &value);
  bool UpdateItem(const std::string &key,
                  const std::string &old_value,
                  const std::string &new_value,
                  const boost::int32_t &ttl,
                  const bool &hashable);
  boost::uint32_t RefreshTime() const;
 private:
  KeyValueIndex key_value_index_;
  // refresh time in seconds
  boost::uint32_t refresh_time_;
  boost::mutex mutex_;
};

}  // namespace kademlia
#endif  // MAIDSAFE_KADEMLIA_DATASTORE_H_
