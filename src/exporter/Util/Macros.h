#pragma once

#include <optional>
#include <memory>
#include <type_traits>


#define StandardTypes(Type) typedef std::unique_ptr<class Type> Type ## UPtr;\
							typedef std::reference_wrapper<class Type> Type ## Ref;\
							typedef std::shared_ptr<class Type> Type ## Ptr;\
							typedef std::optional<class Type> Type ## Opt;

#define StandardTypesStruct(Type) typedef std::unique_ptr<struct Type> Type ## UPtr;\
							typedef std::reference_wrapper<struct Type> Type ## Ref;\
							typedef std::shared_ptr<struct Type> Type ## Ptr;\
							typedef std::optional<struct Type> Type ## Opt;