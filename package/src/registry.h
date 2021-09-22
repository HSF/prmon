// Copyright (C) 2018-2020 CERN
// License Apache2 - see LICENCE file

// Implementation of a class registry that is used
// for all prmon monitoring classes

#ifndef PRMON_REGISTRY_H
#define PRMON_REGISTRY_H

#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "spdlog/spdlog.h"

namespace registry {

// Utility macros
#define REGISTRY_SPACE

#define REGISTRY_CTOR(Base, Derived, params, param_names) \
  [](params) -> Base* { return new Derived(param_names); }

#define REGISTER_MONITOR(Base, Derived, Description) \
  static bool _registered_##Derived =                \
      registry::Registry<Base>::register_class(      \
          #Derived, REGISTRY_CTOR(Base, Derived, , ), Description);

#define REGISTER_MONITOR_1ARG(Base, Derived, Description, dtype1)         \
  static bool _registered_##Derived =                                     \
      registry::Registry<Base, dtype1>::register_class(                   \
          #Derived,                                                       \
          REGISTRY_CTOR(Base, Derived, dtype1 REGISTRY_SPACE arg1, arg1), \
          Description);

// Empty string to return if there is no description
static std::string empty_desc = "";

template <typename T, typename... Pack>
class Registry {
 public:
  using ctor_t = std::function<T*(Pack...)>;
  using map_t = std::map<std::string, ctor_t>;
  using desc_t = std::map<std::string, std::string>;

  template <typename... Args>
  static std::unique_ptr<T> create(const std::string& class_name,
                                   Args&&... pack) {
    if (ctors().count(class_name) == 1) {
      return std::unique_ptr<T>(
          ctors()[class_name](std::forward<Args>(pack)...));
    }
    spdlog::error("Registry: class " + class_name + " is not registered.");
    return std::unique_ptr<T>(nullptr);
  }

  static bool register_class(const std::string& class_name, const ctor_t& ctor,
                             const std::string& description) {
    ctors()[class_name] = ctor;
    desc()[class_name] = description;
    return true;
  }

  static bool is_registered(const std::string& class_name) {
    return ctors().count(class_name) == 1;
  }

  static void unregister(const std::string& class_name) {
    ctors().erase(class_name);
    desc().erase(class_name);
  }

  static std::vector<std::string> list_registered() {
    std::vector<std::string> list{};
    for (const auto& k : ctors()) {
      list.push_back(k.first);
    }
    return list;
  }

  static const std::string& get_description(const std::string& name) {
    if (desc().count(name) == 1) {
      return desc()[name];
    }
    return registry::empty_desc;
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

  Registry();
  Registry(const Registry& other);
  Registry& operator=(const Registry& other);
};

}  // namespace registry

#endif  // PRMON_REGISTRY_H
