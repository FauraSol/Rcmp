#include <list>
#include <unordered_map>
// #include <priority_queue>
#include <string>
#include <type_traits>
#include <vector>

// class eq_item{
// public:
//     std::unordered_map<std::string, std::string> map;
// };

namespace heat_prediction{

using eq_item = std::unordered_map<std::string, std::string>;

enum{
    NO_RETURN = 0,
    RETURN = 1
};

static std::vector<std::string> features = {
    "",
};

template <typename Key, typename Feature>
class OrderedDict {
public:
    using KeyList = std::list<Key>;
    using KeyMapIter = typename KeyList::iterator;
    using MapType = std::unordered_map<Key, std::pair<Feature, KeyMapIter>>;

    void init_value(Feature& val){
        
    }
    
    void update_value(Feature& val){
        for(const auto &feature:features){
            if(!val.count(feature)){
                printf("feature %s not exist\n", feature.c_str());
                abort();
            }
        }
        //TODO update values
    }

    void put_or_update(const Key& key, const Feature& value) {
        if (map.count(key) > 0) {
            // 如果 key 已存在,则更新值
            auto& [val, iter] = map[key];
            update_value(val);
        } else {
            // 如果 key 不存在,则添加新的键值对
            keys.push_back(key);
            map[key] = {value, --keys.end()};
        }
    }

    Feature get(const Key& key) const {
        return map.at(key).first;
    }

    void erase(const Key& key) {
        if (map.count(key) > 0) {
            auto [val, iter] = map[key];
            keys.erase(iter);
            map.erase(key);
        }
    }

    Feature pop() {
        if(keys.size() == 0){
            return Feature();
        }
        auto key = keys.back();
        auto [val, iter] = map[key];
        keys.pop_back();
        map.erase(key);
        return val;
    }

    bool contains(const Key& key) const {
        return map.count(key) > 0;
    }

    typename KeyList::iterator begin() {
        return keys.begin();
    }

    typename KeyList::iterator end() {
        return keys.end();
    }

    size_t size() const {
        return keys.size();
    }

private:
    KeyList keys;
    MapType map;
    
};

template <typename Key, typename Feature=eq_item>
class Evaluation_Queue{
public:
    Evaluation_Queue(){}
    Evaluation_Queue(size_t max_size,  double hot_thred, bool is_training, double alpha, double heat) :
        max_size_(max_size), ts_(0), hot_thred_(hot_thred), is_training_(is_training), alpha_(alpha), heat_(heat) {
        
    }
    ~Evaluation_Queue(){};

    int enqueue(const Key& key, const Feature& value, Feature& out_param);
    int dequeue(Feature& out_param);
    Feature& get(const Key& key);

public:
    
    size_t max_size_;
    uint64_t ts_;
    double hot_thred_;
    bool is_training_;
    double alpha_;
    double heat_;

private:
    OrderedDict<Key, Feature> eq;
    
};

}

