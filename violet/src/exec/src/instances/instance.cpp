#include <vector>
#include <string>
#include <ranges>

#include "executor/instances/instance.h"
#include "executor/process.h"
#include "game/offsets.hpp"

Instance::Instance(uintptr_t address, const Process* process) : address(address), process(process) {}
Instance::Instance(const Instance& instance) : address(instance.address), process(instance.process) {}

uintptr_t Instance::Self() const {
	return address;
}

std::string Instance::Name() const {
	uintptr_t name_ptr = ReadFrom<uintptr_t>(offsets::Instance::Name);

	std::string name = "";

	try {
		name = process->Read<std::string>(name_ptr);
	}
	catch (...) {}

	return name;
}

std::string Instance::ClassName() const {
	uintptr_t classdescriptor = ReadFrom<uintptr_t>(offsets::Instance::ClassDescriptor);
	uintptr_t classname_ptr = process->Read<uintptr_t>(classdescriptor + offsets::Instance::ClassName);

	if (!classname_ptr) {
		return "";
	}

	std::string classname = "";

	try {
		classname = process->Read<std::string>(classname_ptr);
	}
	catch (...) {}

	return classname;
}

Instance Instance::Parent() const {
	uintptr_t parent = ReadFrom<uintptr_t>(offsets::Instance::Parent);
	return Instance(parent, process);
}

std::vector<Instance> Instance::GetChildren() const {
	std::vector<Instance> children;

	uintptr_t children_start = ReadFrom<uintptr_t>(offsets::Instance::ChildrenStart);

	if (!children_start)
		return children;

	uintptr_t children_end = process->Read<uintptr_t>(children_start + offsets::Instance::ChildrenEnd);

	if (!children_end)
		return children;

	uintptr_t children_list = process->Read<uintptr_t>(children_start);

	for (; children_list < children_end; children_list += 0x10) {
		uintptr_t child = process->Read<uintptr_t>(children_list);
		if (child) {
			children.emplace_back(child, process);
		}
	}

	return children;
}

Instance Instance::FindFirstChild(const std::string_view name) const {
	if (!IsValid())
		return Instance();

	for (const auto& child : GetChildren()) {
		if (child.Name() == name)
			return child;

	}

	return Instance();
}

Instance Instance::FindFirstDescendant(const std::string_view name) const {
	if (!IsValid())
		return Instance();

	for (const auto& child : GetDescendants()) {
		if (child.Name() == name)
			return child;
	}

	return Instance();
}

Instance Instance::FindFirstChildOfClass(const std::string_view classname) const {
	if (!IsValid())
		return Instance();

	for (const auto& child : GetChildren()) {
		if (child.ClassName() == classname)
			return child;
	}

	return Instance();
}

Instance Instance::FindFirstChildFromPath(const std::string_view path) const {
	size_t pos = path.rfind('.');

	if (pos == std::string_view::npos) {
		return Instance();
	}

	std::string_view last_node = path.substr(pos + 1);
	Instance last{};

	for (size_t start = 0, end = 0; end != std::string_view::npos; start = end + 1) {
		end = path.find('.', start);
		std::string token(path.substr(start, (end == std::string_view::npos) ? std::string_view::npos : end - start));

		if (token.empty()) break;

		if (!last.IsValid())
			last = FindFirstChild(token);
		else
			last = last.FindFirstChild(token);

		if (!last.IsValid()) return Instance();
	}

	return (last.Name() == last_node) ? last : Instance();
}

Instance Instance::FindFirstChildWhichIsA(const std::string_view classname) const {
	if (!IsValid())
		return Instance();

	for (const auto& child : GetChildren()) {
		if (child.IsA(classname))
			return child;
	}

	return Instance();
}

bool Instance::IsValid() const {
	if (!address)
		return false;
	return true;
}

bool Instance::IsA(const std::string_view classname) const {
	uintptr_t classdescriptor = ReadFrom<uintptr_t>(offsets::Instance::ClassDescriptor);

	while (true) {
		if (!classdescriptor) break;

		uintptr_t base_classname_ptr = process->Read<uintptr_t>(classdescriptor + offsets::Instance::ClassName);

		if (!base_classname_ptr) break;

		try {
			std::string base_classname = process->Read<std::string>(base_classname_ptr);

			if (base_classname == classname)
				return true;

			if (base_classname == "<<<ROOT>>>")
				return false;
		}
		catch (...) {}

		classdescriptor = process->Read<uintptr_t>(classdescriptor + offsets::Instance::ClassBase);
	}

	return false;
}

std::vector<Instance> Instance::GetDescendants() const {
	std::vector<Instance> descendants;

	for (const auto& child : GetChildren()) {
		std::vector<Instance> child_descendants = child.GetDescendants();

		descendants.push_back(child);
		descendants.insert(descendants.end(),
			std::make_move_iterator(child_descendants.begin()),
			std::make_move_iterator(child_descendants.end()));
	}

	return descendants;
}
