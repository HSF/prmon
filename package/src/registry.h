// Copyright (C) CERN, 2020

#ifndef PRMON_REGISTRY_H
#define PRMON_REGISTRY_H

#include <functional>
#include <iostream>
#include <string>
#include <map>
#include <vector>

namespace registry {

// Utility macros
#define REGISTRY_CTOR(Base, Derived, params, param_names) \
  [](params) -> Base* { return new Derived(param_names); }
    
#define REGISTER_MONITOR(Base, Derived, Identifier, Description) \
  static bool _registered_##Derived = \
      registry::Registry<Base>::Register(Identifier, \
          REGISTRY_CTOR(Base, Derived,,), Description);


static std::string empty = "";

template<typename T, typename ... Pack>
class Registry {
 public:
  using ctor_t = std::function<T*(Pack...)>;
  using map_t = std::map<std::string, ctor_t>;
  using desc_t = std::map<std::string, std::string>;

  template<typename ... Args>
  static T* Create(const std::string& class_name, Args && ... pack) {
    if (ctors().count(class_name) == 1) {
      return ctors()[class_name](std::forward<Args>(pack)...);
    }
    std::cerr << "Class " << class_name << " not registered." << std::endl;
    return nullptr;
  }

  static bool Register(const std::string& class_name, const ctor_t& ctor, const std::string& description) {
    ctors()[class_name] = ctor;
    desc()[class_name] = description;
    return true;
  }

  static bool IsRegistered(const std::string& class_name) {
    return ctors().count(class_name) == 1;
  }

  static void Unregister(const std::string& class_name) {
    ctors().erase(class_name);
    desc().erase(class_name);
  }

  static std::vector<std::string> ListRegistered() {
    std::vector<std::string> list{};
    for (const auto k : ctors()) {
      list.push_back(k.first);
    }
    return list;
  }

  static const std::string& GetDescription(const std::string& name) {
    if (desc().count(name) == 1) {
      return desc()[name];
    }
    return registry::empty;
  }

 private:
  static map_t& ctors() {
    static map_t ctor_map;
    return ctor_map;
  }

  static desc_t& desc() {
    static desc_t desc_map;
    return desc_map;
  }
  // static const& std::string empty() {
  //   static std::string empty_string{};
  //   return empty_string;
  // }

  Registry();
  Registry(const Registry& other);
  Registry& operator=(const Registry& other);
};

}

#endif // PRMON_REGISTRY_H
