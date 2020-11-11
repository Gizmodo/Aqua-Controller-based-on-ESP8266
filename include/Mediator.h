#ifndef FACTORYMETHOD_MEDIATOR2_H
#define FACTORYMETHOD_MEDIATOR2_H

#include <map>
#include <list>
#include <string>

template<typename T>
class Mediator {
    typedef void (*Action)(T);

private:
    std::multimap<std::string, std::list<Action> > actions;

public:
    Mediator() = default;

    ~Mediator() = default;

    void Register(std::string message, Action action) {
        typename std::multimap<std::string, std::list<Action> >::iterator iter_ = actions.find(message);

        if (iter_ != actions.end()) {
            iter_->second.push_back(action);
        } else {
            std::list<Action> list;
            list.push_back(action);
            actions.insert(std::pair<std::string, std::list<Action> >(message, list));
        }
    }

    void UnRegister(std::string message, Action action) {
        typename std::multimap<std::string, std::list<Action> >::iterator iter_ = actions.find(message);

        if (iter_ != actions.end()) {
            iter_->second.remove(action);
        }
    }

    void Send(std::string message, T param) {
        typename std::multimap<std::string, std::list<Action> >::iterator iter_ = actions.find(message);

        if (iter_ != actions.end()) {
            typename std::list<Action>::iterator listIter_ = iter_->second.begin();

            while (listIter_ != iter_->second.end()) {
                (*listIter_)(param);
                listIter_++;
            }
        }
    }
};

#endif //FACTORYMETHOD_MEDIATOR2_H
