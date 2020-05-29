// Copyright (C) CERN, 2020

#ifndef PRMON_REGISTRY_H
#define PRMON_REGISTRY_H

#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>

namespace registry {

// Utility macros
#define REGISTRY_SPACE
#define REGISTRY_CONCAT(...) __VA_ARGS__
#define REGISTRY_GET_MACRO(_0, _1, _2, _3, _4, NAME, ...) NAME

#define REGISTER_SUBCLASS_W_IDENTIFIER(Base, Derived, Identifier, ...) \
    REGISTRY_GET_MACRO(_0, ##__VA_ARGS__, \
        REGISTER_SUBCLASS4, \
        REGISTER_SUBCLASS3, \
        REGISTER_SUBCLASS2, \
        REGISTER_SUBCLASS1, \
        REGISTER_SUBCLASS0)(Base, Derived, #Identifier, ##__VA_ARGS__)

#define REGISTER_SUBCLASS(Base, Derived, ...) \
    REGISTER_SUBCLASS_W_IDENTIFIER(Base, Derived, Derived, ##__VA_ARGS__)

#define REGISTRY_CTOR(Base, Derived, params, param_names) \
  [](params) -> Base* { return new Derived(param_names); }
    
#define REGISTER_SUBCLASS0(Base, Derived, Identifier) \
  static bool _registered_##Derived = \
      registry::Registry<Base>::Register(Identifier, \
          REGISTRY_CTOR(Base, Derived,,));

#define REGISTER_SUBCLASS1(Base, Derived, Identifier, dtype1) \
  static bool _registered_##Derived = \
      registry::Registry<Base, dtype1>::Register(Identifier, \
          REGISTRY_CTOR(Base, Derived, \
              dtype1 REGISTRY_SPACE arg1, arg1));

#define REGISTER_SUBCLASS2(Base, Derived, Identifier, dtype1, dtype2) \
  static bool _registered_##Derived = \
      registry::Registry<Base, dtype1, dtype2>::Register(Identifier, \
          REGISTRY_CTOR(Base, Derived, \
              REGISTRY_CONCAT(dtype1 REGISTRY_SPACE arg1, \
                              dtype2 REGISTRY_SPACE arg2), \
              REGISTRY_CONCAT(arg1, arg2)));

#define REGISTER_SUBCLASS3(Base, Derived, Identifier, dtype1, dtype2, dtype3) \
  static bool _registered_##Derived = \
      registry::Registry<Base, dtype1, dtype2, dtype3>::Register(Identifier, \
          REGISTRY_CTOR(Base, Derived, \
              REGISTRY_CONCAT(dtype1 REGISTRY_SPACE arg1, \
                              dtype2 REGISTRY_SPACE arg2, \
                              dtype3 REGISTRY_SPACE arg3), \
              REGISTRY_CONCAT(arg1, arg2, arg3)));

#define REGISTER_SUBCLASS4(Base, Derived, Identifier, dtype1, dtype2, dtype3, \
                           dtype4) \
  static bool _registered_##Derived = \
      registry::Registry<Base, dtype1, dtype2, dtype3, dtype4>::Register( \
          Identifier, \
          REGISTRY_CTOR(Base, Derived, \
              REGISTRY_CONCAT(dtype1 REGISTRY_SPACE arg1, \
                              dtype2 REGISTRY_SPACE arg2, \
                              dtype3 REGISTRY_SPACE arg3, \
                              dtype4 REGISTRY_SPACE arg4), \
              REGISTRY_CONCAT(arg1, arg2, arg3, arg4)));

template<typename T, typename ... Pack>
class Registry {
 public:
  using ctor_t = std::function<T*(Pack...)>;
  using map_t = std::unordered_map<std::string, ctor_t>;

  template<typename ... Args>
  static T* Create(const std::string& class_name, Args && ... pack) {
    if (ctors().count(class_name) == 1) {
      return ctors()[class_name](std::forward<Args>(pack)...);
    }
    std::cerr << "Class " << class_name << " not registered." << std::endl;
    return nullptr;
  }

  static bool Register(const std::string& class_name, const ctor_t& ctor) {
    ctors()[class_name] = ctor;
    return true;
  }

  static bool IsRegistered(const std::string& class_name) {
    return ctors().count(class_name) == 1;
  }

  static void Unregister(const std::string& class_name) {
    ctors().erase(class_name);
  }

  const map_t& get_map() {
    static map_t ctor_map;
    return ctor_map;
  }

 private:
  static map_t& ctors() {
    static map_t ctor_map;
    return ctor_map;
  }

  Registry();
  Registry(const Registry& other);
  Registry& operator=(const Registry& other);
};

}

#endif // PRMON_REGISTRY_H
