#include "evaluation_queue.hpp"

namespace heat_prediction{
template <typename Key, typename Feature>
int Evaluation_Queue<Key,Feature>::enqueue(const Key& key, const Feature& value, Feature& out_param) {
    int ret = -1;
    if(eq.size() >= max_size_){
        ret = dequeue(out_param);
    }
    return eq.put_or_update(key, value);
}
template <typename Key, typename Feature>
int Evaluation_Queue<Key,Feature>::dequeue(Feature & out_param){
    auto val = eq.pop();
    if(val == Feature()){
        return NO_RETURN;
    }
    out_param = val;
    return RETURN;
}

}