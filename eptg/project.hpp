#ifndef PROJECT_HPP
#define PROJECT_HPP

#include <vector>
#include <set>
#include <map>
#include <list>
#include <memory>
#include <algorithm>
#include <variant>
#include <fstream>
#include <locale>
#include <experimental/map>

#include "eptg/search.hpp"
#include "eptg/helpers.hpp"
#include "eptg/constants.hpp"
#include "eptg/path.hpp"
#include "eptg/json.hpp"
#include "eptg/copy_move_data.hpp"
#include "eptg/fs.hpp"
#include "eptg/string.hpp"

namespace eptg {

template<typename STR>
struct taggable
{
    std::set<STR> inherited_tags;

    inline bool  insert_tag(const STR & tag)       { return inherited_tags.insert(tag).second; }
    inline size_t erase_tag(const STR & tag)       { return inherited_tags.erase (tag); }
    inline bool     has_tag(const STR & tag) const { return inherited_tags.find  (tag) != inherited_tags.end(); }
    template<typename C>
    bool has_all_of(const C & tags) const
    {
        return std::find_if(tags.begin(), tags.end(), [this](const STR & t){ return ! in(inherited_tags, t); }) == tags.end();
    }
    template<typename C>
    bool has_one_of(const C & tags) const
    {
        return std::find_if(tags.begin(), tags.end(), [this](const STR & t){ return in(inherited_tags, t); }) != tags.end();
    }

	size_t size() const
	{
		return inherited_tags.size();
	}

	bool operator==(const taggable & other) const
	{
		return inherited_tags == other.inherited_tags;
	}
	bool operator!=(const taggable & other) const
	{
		return inherited_tags != other.inherited_tags;
	}
};

template<typename T> using File = taggable<T>;
template<typename T> using Tag  = taggable<T>;

template<typename STR, typename T>
struct taggable_collection
{
    std::map<STR,T> collection;

    inline size_t size() const { return collection.size(); }
    inline size_t count_tagged() const
    {
        return std::count_if(collection.begin(), collection.end(), [](const auto & p) { return p.second.inherited_tags.size() > 0; });
    }

	bool operator==(const taggable_collection & other) const
	{
		if (collection == other.collection)
			return true;
		for (const auto & [str,t] : collection)
		{
			if (t.size() == 0)
				continue;
			auto it = other.collection.find(str);
			if (it == other.collection.end() || t != it->second)
				return false;
		}
		for (const auto & [str,t] : other.collection)
		{
			if (t.size() == 0)
				continue;
			auto it = collection.find(str);
			if (it == collection.end() || t != it->second)
				return false;
		}
		return true;
	}
	bool operator!=(const taggable_collection & other) const
	{
		return ! (*this == other);
	}

    const T * find(const STR & id) const
    {
        auto it = collection.find(id);
        if (it == collection.end())
            return nullptr;
        return &it->second;
    }
    T * find  (const STR & id)         { return const_cast<T*>(const_cast<const taggable_collection<STR,T>*>(this)->find(id)); }
    T & insert(const STR & id, T && t) { return collection.insert({id, t}).first->second; }
    bool has  (const STR & id) const   { return collection. find(id) != collection.end(); }
    bool erase(const STR & id)         { return collection.erase(id)>0; }
    std::map<STR,T> get_tagged_with_all(const std::set<STR> & tags) const
    {
        std::map<STR,T> result;
        for (const auto & p : collection)
        {
            if ( ! p.second.has_all_of(tags))
                continue;
            result.insert(p);
        }
        return result;
    }
    std::set<const taggable<STR>*> get_all_by_name(const std::set<STR> & ids) const
    {
        std::set<const taggable<STR>*> result;
        for (const STR & id : ids)
        {
            const T * t = find(id);
            if (t != nullptr)
                result.insert(t);
        }
        return result;
    }
};

template<typename STR>
class Project
{
private:
    STR path;
    taggable_collection<STR,File<STR>> files;
    taggable_collection<STR,Tag <STR>> tags ;
	bool needs_saving = false;

public:
	explicit inline Project()
	{}

	Project(const STR & path, const std::wstring & json_string)
		: path(path)
	{
		if (json_string.size() == 0)
			return;
		const wchar_t * c = json_string.c_str();
		eptg::json::dict<STR> dict = eptg::json::read_dict<STR>(c);
		if ( ! in(dict, "files") || ! std::holds_alternative<eptg::json::dict<STR>>(dict["files"]))
			return;
		if ( ! in(dict, "tags" ) || ! std::holds_alternative<eptg::json::dict<STR>>(dict["tags" ]))
			return;
		for(const auto & [rel_path,file_var] : std::get<eptg::json::dict<STR>>(dict["files"]))
		{
			if ( ! std::holds_alternative<json::dict<STR>>(file_var))
				continue;
			json::dict<STR> file_dict = std::get<json::dict<STR>>(file_var);
			File<STR> & file = files.insert(rel_path, File<STR>());
			if ( ! in(file_dict, "tags" ) || ! std::holds_alternative<json::array<STR>>(file_dict["tags" ]))
				continue;
			for (const auto & tagname_var : std::get<json::array<STR>>(file_dict["tags"]))
			{
				if ( ! std::holds_alternative<STR>(tagname_var))
					continue;
				const STR & tagname = std::get<STR>(tagname_var);
				if (tagname.size() > 0)
				{
					file.insert_tag(tagname);
					tags.insert(tagname, Tag<STR>());
				}
			}
		}
		for(const auto & [tag_name,tag_var] : std::get<json::dict<STR>>(dict["tags"]))
		{
			Tag<STR> & tag = tags.insert(tag_name, Tag<STR>());
			if ( ! std::holds_alternative<json::dict<STR>>(tag_var))
				continue;
			json::dict<STR> tag_dict = std::get<json::dict<STR>>(tag_var);
			if ( ! in(tag_dict, "tags"))
				continue;
			for (const auto subtagname_var : std::get<json::array<STR>>(tag_dict["tags"]))
			{
				if ( ! std::holds_alternative<STR>(subtagname_var))
					continue;
				const STR & subtagname = std::get<STR>(subtagname_var);
				if (subtagname.size() > 0)
					tag.insert_tag(subtagname);
			}
		}
	}

	bool operator==(const Project & other) const
	{
		bool a = files == other.files;
		bool b = tags  == other.tags;
		return a
			&& b;
	}
	bool operator!=(const Project & other) const
	{
		return ! (*this == other);
	}

	const STR & get_path() const { return path; }
	const decltype(files) & get_files       () const {                         return files; }
		  decltype(files) & get_files_modify()       { set_needs_saving(true); return files; }
	const decltype(files) & get_tags        () const {                         return tags ; }
		  decltype(files) & get_tags_modify ()       { set_needs_saving(true); return tags ; }
	bool get_needs_saving() const { return needs_saving; }
	void set_needs_saving(bool f)
	{
		needs_saving |= f;
	}
	void clear_needs_saving() { needs_saving = false; }

    bool absorb(const Project & sub_project)
    {
		if ( ! path::is_sub(path, sub_project.get_path()))
            return false;

		set_needs_saving(true);

		STR sub_project_rel_path = path::relative(path, sub_project.get_path());

        std::set<STR> tags_used;
		for (const auto & [rel_path,file] : sub_project.get_files().collection)
        {
            const STR new_rel_path = path::append(sub_project_rel_path, rel_path);
            files.insert(new_rel_path, eptg::taggable(file));
            tags_used.insert(file.inherited_tags.begin(), file.inherited_tags.end());
        }

        std::set<STR> tags_used_done;
        while( ! tags_used.empty())
        {
            const STR & tag = *tags_used.begin();
            const eptg::Tag<STR> * tag_in_sub_project = sub_project.tags.find(tag);
                  eptg::Tag<STR> & tag_in_new_project =       this->tags.insert(tag, eptg::Tag<STR>{});

            if (tag_in_sub_project)
                for (const STR & inherited_tag : tag_in_sub_project->inherited_tags)
                {
                    tag_in_new_project.insert_tag(inherited_tag);
                    if (tags_used_done.find(inherited_tag) == tags_used_done.end())
                        tags_used.insert(inherited_tag);
                }

            tags_used_done.insert(tag);
            tags_used.erase(tag);
        }

        return true;
    }

    template<typename C>
    std::set<STR> get_descendent_tags(const C & p_tags) const
    {
        std::set<STR> result;
        for (const auto & [name,tag] : this->tags.collection)
            if (tag.has_one_of(p_tags))
                result.insert(name);
        return result;
    }
    std::map<STR,const File<STR>*> get_files_tagged_with_all(const std::set<STR> & tags) const
    {
        std::map<STR,const File<STR>*> result;
        for (const auto & [id,f] : files.collection)
            if (inherits(f, tags))
                result.insert({id, &f});
        return result;
    }
    std::set<STR> get_common_tags(const std::set<const taggable<STR>*> & taggables) const
    {
        std::set<STR> result;
        if (taggables.size() == 0)
            return result;

        auto it = taggables.begin();
        auto end = taggables.end();

        // get tags for 1st taggable as baseline
        if (*it == nullptr)
            return std::set<STR>();
        result = (*it)->inherited_tags;

        // filter out tags that are not in subsequent taggables
        for ( ++it
            ; it != end && result.size() > 0
            ; ++it)
        {
            if (*it == nullptr)
                return std::set<STR>();
            decltype(result) tmp;
            std::set_intersection(result.begin(), result.end(),
                                  (*it)->inherited_tags.begin(), (*it)->inherited_tags.end(),
                                  std::inserter(tmp, tmp.end()));
            result = tmp;
        }

        return result;
    }
    bool inherits(const taggable<STR> & taggable, std::set<STR> tags_to_have) const
    {
        for ( std::set<STR> tags = taggable.inherited_tags
            ; tags.size() > 0 && tags_to_have.size() > 0
            ; tags = get_common_tags(this->tags.get_all_by_name(tags)) )
        {
            for (const STR & tag : tags)
                tags_to_have.erase(tag);
        }
        return tags_to_have.size() == 0;
    }

    std::vector<STR> search(const SearchNode<STR> & search_node) const
    {
        std::vector<STR> matches;

        for (const auto & p : this->files.collection)
            if (search_node.eval(this->get_all_inherited_tags({&p.second})))
                matches.push_back(p.first);

        return matches;
    }
    std::set<STR> get_all_inherited_tags(const std::set<const taggable<STR>*> & taggables) const
    {
        std::set<STR> result{};
        for (const taggable<STR> * taggable : taggables)
            for (const STR & tag : taggable->inherited_tags)
                result.insert(tag).second;
        for (std::set<STR> new_tags = result ; ! new_tags.empty() ; )
        {
            const STR tag = *new_tags.begin();
            new_tags.erase(new_tags.begin());
            for (const STR & tag : get_all_inherited_tags(this->tags.get_all_by_name({tag})))
                if (result.insert(tag).second)
                    new_tags.insert(tag);
        }
        return result;
    }
    std::vector<std::vector<STR>> _get_tag_paths(const STR & tag, const STR & top_tag) const
    {
        std::vector<std::vector<STR>> result;

        const taggable<STR> * t = tags.find(tag);
        if ( ! t)
            return result;

        if (t->has_tag(top_tag))
            return {{}};

        for (const STR & tag : t->inherited_tags)
            for (std::vector<STR> & path : _get_tag_paths(tag, top_tag))
            {
                path.insert(path.begin(), tag);
                result.push_back(std::move(path));
            }
        return result;
    }
    std::vector<std::vector<STR>> get_tag_paths(const STR & rel_path, const STR & top_tag) const
    {
        std::vector<std::vector<STR>> result;

        const taggable<STR> * f = files.find(rel_path);
        if ( ! f)
            return result;

        if (f->has_tag(top_tag))
            return {};

        for (const STR & tag : f->inherited_tags)
            for (std::vector<STR> & path : _get_tag_paths(tag, top_tag))
            {
                path.insert(path.begin(), tag);
                result.push_back(std::move(path));
            }
        return result;
    }

    template<bool ISTAG>
    void set_common_tags(const std::set<STR> & taggables_names
                        ,const std::set<STR> & typed_tags
                        ,std::function<void(const STR&,int)> tag_count_callback
                        ,std::function<void(const STR & selected_tag_name, const STR & st)> tag_loop_callback = [](const STR&, const STR&){}
                        )
    {
        std::set<STR> common_tags  = get_common_tags((ISTAG?tags:files).get_all_by_name(taggables_names));
        std::set<STR> added_tags   = added(common_tags, typed_tags);
        std::set<STR> removed_tags = added(typed_tags, common_tags);

        if (added_tags.empty() && removed_tags.empty())
            return;

		set_needs_saving(true);

        for (const STR & taggable_name : taggables_names)
        {
            taggable<STR> * tagbl = (ISTAG?tags:files).find(taggable_name);

            if (removed_tags.size() > 0)
                for (const STR & t : removed_tags)
                    tag_count_callback(t, - tagbl->erase_tag(t));
            if (added_tags.size() > 0)
            {
                for (const STR & t : added_tags)
                {
                    if (ISTAG)
                    {
                        // let's check that the tag added does not already inherit (is tagged with) the selected tag
                        // if so, it would create circular inheritance, which does not make sense
                        // e.g.: if TZM is a ACTIVISM. Can't make ACTIVISM a TZM.
                        eptg::Tag<STR> * candidate = tags.find(t);
                        if (  taggable_name == t // self-tagging
                           || (candidate != nullptr && inherits(*candidate, {taggable_name}))
                           )
                            tag_loop_callback(taggable_name, t);
                        else
                           tag_count_callback(t, tagbl->insert_tag(t));
                    }
                    else
                        tag_count_callback(t, tagbl->insert_tag(t));
                }
            }
        }
    }

    // returns whether a new project has been created in destination folder.
	Project execute(const eptg::CopyMoveData<STR> & copy_move_data)
	{
		Project dest_project(copy_move_data.dest, read_file(path::append(copy_move_data.dest, PROJECT_FILE_NAME)));
        std::set<STR> tags_used;

        bool is_internal = path::is_sub(path, copy_move_data.dest);

		set_needs_saving(is_internal);
		set_needs_saving(copy_move_data.is_move);

        for (const auto & filenames_tuple : copy_move_data.files)
        {
            STR new_rel_path = std::get<0>(filenames_tuple);
            STR old_rel_path = std::get<1>(filenames_tuple);

            eptg::fs::create_directories(path::append(eptg::str::to<std::string>(copy_move_data.dest), eptg::fs::path(eptg::str::to<std::string>(new_rel_path)).parent_path()));

            if ( ! eptg::fs::exists(eptg::str::to<std::string>(path::append(path , old_rel_path))))
                continue;

            // actual files
            if (copy_move_data.is_move)
                eptg::fs::rename(eptg::str::to<std::string>(path::append(path , old_rel_path))
                                ,eptg::str::to<std::string>(path::append(copy_move_data.dest, new_rel_path))
                                );
            else
                eptg::fs::copy(eptg::str::to<std::string>(path::append(path , old_rel_path))
                              ,eptg::str::to<std::string>(path::append(copy_move_data.dest, new_rel_path))
                              );

            // files in Project
			eptg::File<STR> & new_file = dest_project.get_files_modify().insert(new_rel_path, eptg::File<STR>{});
            const eptg::File<STR> * old_file = files.find(old_rel_path);

            // tags in Project
            for (const STR & tag : old_file->inherited_tags)
            {
                new_file.insert_tag(tag);
                if ( ! is_internal)
                    tags_used.insert(tag);
            }

            if (copy_move_data.is_move)
                files.erase(old_rel_path);
        }

        // tag inheritance
        std::set<STR> tags_used_done;
        while( ! tags_used.empty())
        {
            const STR & tag = *tags_used.begin();
            const eptg::Tag<STR> * tag_in_old_project = tags.find(tag);
				  eptg::Tag<STR> & tag_in_new_project = dest_project.get_tags_modify().insert(tag, eptg::Tag<STR>{});

            if (tag_in_old_project)
                for (const STR & inherited_tag : tag_in_old_project->inherited_tags)
                {
                    tag_in_new_project.insert_tag(inherited_tag);
                    if (tags_used_done.find(inherited_tag) == tags_used_done.end())
                        tags_used.insert(inherited_tag);
                }

            tags_used_done.insert(tag);
            tags_used.erase(tag);
        }
        return dest_project;
    }

    enum RenameStatus
    {
        Success = 0,
        DifferentFolders,
        OriginNotFound,
        DestinationExists,
    };

    RenameStatus rename(const STR & old_rel_path, const STR & new_rel_path)
    {
        if (eptg::fs::path(eptg::str::to<std::string>(old_rel_path)).parent_path() != eptg::fs::path(eptg::str::to<std::string>(new_rel_path)).parent_path())
            return RenameStatus::DifferentFolders;

        auto it = files.collection.find(old_rel_path);
        if (it == files.collection.end() || ! eptg::fs::exists(eptg::str::to<std::string>(path::append(path, old_rel_path))))
            return RenameStatus::OriginNotFound;

        if (files.has(new_rel_path))
            return RenameStatus::DestinationExists;

		set_needs_saving(true);

        STR old_full_path = path::append(path, old_rel_path);
        STR new_full_path = path::append(path, new_rel_path);

        eptg::fs::rename(eptg::str::to<std::string>(old_full_path), eptg::str::to<std::string>(new_full_path));
        File<STR> file = it->second; // save a copy
        files.collection.erase(it);
        files.collection.insert({new_rel_path, file});

        return RenameStatus::Success;
    }

	void sweep()
	{
		// prune deleted files
		std::experimental::erase_if(files.collection, [this](const auto & p){ return ! eptg::fs::exists(eptg::str::to<std::string>(path::append(path, p.first))); });
		// sweep directory
		if ( ! eptg::fs::exists(eptg::str::to<std::string>(path)))
			return;
		for (const STR & str : path::sweep(path, std::set<STR>{".jpg", ".jpeg", ".png", ".gif", ".bmp", ".pbm", ".pgm", ".ppm", ".xbm", ".xpm", ".txt"}))
			files.insert(str, File<STR>());
	}

	bool save(bool force = false)
	{
		bool saved = const_cast<const Project*>(this)->save(force);
		needs_saving = false;
		return saved;
	}
	bool save(bool force = false) const
	{
		if ( ! get_needs_saving() && ! force)
			return true;

		eptg::json::dict<STR> files_dict;
		for (const auto & [id,file] : this->files.collection)
		{
			if (file.inherited_tags.size() == 0)
				continue;
			eptg::json::array<STR> tags_array;
			for (const auto & tag : file.inherited_tags)
				tags_array.push_back(tag);
			if (tags_array.empty())
				continue;
			eptg::json::dict<STR> ok;
			ok.insert({"tags",tags_array});
			files_dict.insert({id,std::move(ok)});
		}
		eptg::json::dict<STR> tags_dict;
		for (const auto & [id,tag] : this->tags.collection)
		{
			if (tag.inherited_tags.size() == 0)
				continue;
			eptg::json::array<STR> tags_array;
			for (const auto & inherited_tag : tag.inherited_tags)
				tags_array.push_back(inherited_tag);
			if (tags_array.empty())
				continue;
			eptg::json::dict<STR> ok;
			ok.insert({"tags",tags_array});
			tags_dict.insert({id,std::move(ok)});
		}
		eptg::json::dict<STR> document;
		document["files"] = files_dict;
		document["tags" ] = tags_dict ;

		STR str = eptg::json::to_str(document, 0);

		Project new_project("", eptg::str::to<std::wstring>(str));
		if (new_project != *this)
			return false;

		std::string json_str = eptg::str::to<std::string>(str);
		std::ofstream out(eptg::str::to<std::string>(path::append(path, PROJECT_FILE_NAME)));
		out.write(json_str.c_str(), json_str.size());
		out.flush();
		out.close();

		return true;
	}
};

} // namespace

#endif // include guard
