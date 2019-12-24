#ifndef COPY_MOVE_DATA_HPP
#define COPY_MOVE_DATA_HPP

#include "eptg/path.hpp"
#include "eptg/fs.hpp"
#include "eptg/helpers.hpp"

namespace eptg {

template<typename STR>
struct CopyMoveData
{
    enum TreeType
    {
        None,
        Preserve,
        Tag,
    };
    enum WhichFiles
    {
        Selected,
        Shown,
        All,
    };

    bool is_move;
    WhichFiles which_files;
    STR dest;
    TreeType tree_type;
    bool overwrite;
    STR tag;

    std::map<STR,STR> files;
    size_t name_collision_count;

    inline CopyMoveData(const STR & base_path, bool move, WhichFiles which_files, STR dest, TreeType tree, bool overwrite, STR tag="")
        : is_move(move)
        , which_files(which_files)
        , dest(path::is_relative(dest) ? path::append(base_path, dest) : std::move(dest))
        , tree_type(tree)
        , overwrite(overwrite)
        , tag(std::move(tag))
        , name_collision_count(0)
    {}

    inline bool operator==(const CopyMoveData & other) const
    {
        return is_move     == other.is_move
            && which_files == other.which_files
            && dest        == other.dest
            && tree_type   == other.tree_type
            && overwrite   == other.overwrite
            && tag         == other.tag
            ;
    }
    inline bool operator!=(const CopyMoveData & other) const
    {
        return !operator==(other);
    }

    template<typename C_all, typename C_shown, typename C_sel, typename Project>
    void process(const Project & project, const C_all & all_files, const C_shown & shown_files, const C_sel & selected_files)
    {
        auto add_file = [this,&project](const STR & rel_path)
            {
                STR new_rel_path;
                if (tree_type == TreeType::None)
                    new_rel_path = get_preview_flat(rel_path);
                else if (tree_type == TreeType::Preserve)
                    new_rel_path = get_preview_preserve(rel_path);
                else if (tree_type == TreeType::Tag)
                    new_rel_path = get_preview_tag(project.get_tag_paths(rel_path, this->tag), rel_path);
                if ( ! overwrite)
                    new_rel_path = adjust_for_duplicity(new_rel_path);
                else
                    count_for_duplicity(new_rel_path); // this updates the duplicity counter
                files.emplace(new_rel_path, rel_path);
            };

        if (which_files == WhichFiles::Selected)
            for (const STR & rel_path : selected_files)
                add_file(rel_path);
        else if (which_files == WhichFiles::Shown)
            for (const STR & rel_path : shown_files)
                add_file(rel_path);
        else if (which_files == WhichFiles::All)
            for (const STR & rel_path : all_files)
                add_file(rel_path);
    }

    inline STR get_preview_flat(const STR & rel_path) const
    {
        return QFileInfo(rel_path).fileName();
    }
    inline STR get_preview_preserve(const STR & rel_path) const
    {
        return rel_path;
    }
    inline STR get_preview_tag(const std::vector<std::vector<STR>> & paths, const STR & rel_path) const
    {
        if (paths.empty())
            return get_preview_flat(rel_path);
        else
        {
            return path::append(
                    std::accumulate(paths[0].rbegin(), paths[0].rend(), STR(""),
                        [](const STR & result, const STR & folder)
                            {
                                if (result == "")
                                    return folder;
                                else
                                    return path::append(result, folder);
                            }
                        ),
                    QFileInfo(rel_path).fileName()
                );
        }
    }

    inline STR adjust_for_duplicity(const STR & rel_path)
    {
        if ( ! eptg::fs::exists(str_to<std::string>(path::append(dest, rel_path)))
          && ! in(files, rel_path))
            return rel_path;
        STR path      = str_to<STR>(eptg::fs::path(str_to<std::string>(rel_path)).parent_path());
        STR filename  = str_to<STR>(eptg::fs::path(str_to<std::string>(rel_path)).stem       ());
        STR extension = str_to<STR>(eptg::fs::path(str_to<std::string>(rel_path)).extension  ());

        int i = 2;
        for (;;)
        {
            STR new_rel_path = path::append(path,filename).append(" (").append(str_to<STR>(std::to_string(i))).append(")").append(extension);
            if ( !  eptg::fs::exists(str_to<std::string>(path::append(dest, new_rel_path)))
              && ! in(files, new_rel_path))
            {
                return new_rel_path;
            }
            i++;
            name_collision_count++;
        }
        return "";
    }
    inline void count_for_duplicity(const STR & new_rel_path)
    {
        if (QFileInfo(path::append(dest, new_rel_path)).exists() || in(files, new_rel_path))
            name_collision_count++;
    }
};

} // namespace

#endif // include guards
