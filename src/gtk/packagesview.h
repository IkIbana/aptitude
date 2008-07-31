// -*-c++-*-

// packagesview.h
//
//  Copyright 1999-2008 Daniel Burrows
//  Copyright 2008 Obey Arthur Liu
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; see the file COPYING.  If not, write to
//  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
//  Boston, MA 02111-1307, USA.

#ifndef PACKAGESVIEW_H_
#define PACKAGESVIEW_H_

#undef OK
#include <gtkmm.h>
#include <libglademm/xml.h>

#include <apt-pkg/error.h>

#include <generic/apt/apt.h>

namespace gui
{
  extern undo_group * undo;

  class PackagesView;

  /**
   * This is a list of packages actions.
   * TODO: This probably already exist. Find it.
   * FIXME: Description shouldn't be here.
   */
  enum PackagesAction
  {
    Install, Remove, Purge, Keep, Hold
  };

  std::string current_state_string(pkgCache::PkgIterator pkg, pkgCache::VerIterator ver);
  std::string selected_state_string(pkgCache::PkgIterator pkg, pkgCache::VerIterator ver);

  /**
   * The PackagesMarker marks packages belonging to a PackagesTab
   */
  class PackagesMarker
  {
    private:
      PackagesView * view;
      void dispatch(pkgCache::PkgIterator pkg, pkgCache::VerIterator ver, PackagesAction action);
      void callback(const Gtk::TreeModel::iterator& iter, PackagesAction action);
    public:
      /** \brief Construct a packages marker for tab.
       *
       *  \param tab The tab on which the marking takes place.
       */
      PackagesMarker(PackagesView * view);
      void select(PackagesAction action);
  };

  /**
   * The context menu for packages in PackagesTab
   */
  class PackagesContextMenu
  {
    private:
      Gtk::Menu * pMenu;
      Gtk::ImageMenuItem * pMenuInstall;
      Gtk::ImageMenuItem * pMenuRemove;
      Gtk::ImageMenuItem * pMenuPurge;
      Gtk::ImageMenuItem * pMenuKeep;
      Gtk::ImageMenuItem * pMenuHold;
    public:
      /** \brief Construct a context menu for tab.
       *
       *  \param tab The tab who owns the context menu.
       *  \param marker The marker to use to execute the actions.
       */
    PackagesContextMenu(PackagesView * view);
    Gtk::Menu * get_menu() const { return pMenu; };
  };

  class PackagesColumns : public Gtk::TreeModel::ColumnRecord
  {
    public:
      Gtk::TreeModelColumn<pkgCache::PkgIterator> PkgIterator;
      Gtk::TreeModelColumn<pkgCache::VerIterator> VerIterator;
      Gtk::TreeModelColumn<bool> BgSet;
      Gtk::TreeModelColumn<Glib::ustring> BgColor;
      Gtk::TreeModelColumn<Glib::ustring> CurrentStatus;
      Gtk::TreeModelColumn<Glib::ustring> SelectedStatus;
      Gtk::TreeModelColumn<Glib::ustring> Name;
      Gtk::TreeModelColumn<Glib::ustring> Section;
      Gtk::TreeModelColumn<Glib::ustring> Version;

      PackagesColumns();

      /** \brief Fill in the contents of a tree-model row for the given
       *  package/version pair.
       *
       *  \param row                 The row to fill in; any existing values
       *                             will be overwritten.
       *  \param pkg                 The package to display in this row.
       *  \param ver                 The version to display in this row.
       *  \param version_specific    The row is version specific (influences
       *                             coloring and selected status display)
       */
      void fill_row(Gtk::TreeModel::Row &row,
                    const pkgCache::PkgIterator &pkg,
                    const pkgCache::VerIterator &ver,
                    bool version_specific = false) const;
      /** \brief Fill in the contents of a tree-model row for a header.
       *
       *  \param row                 The row to fill in; any existing values
       *                             will be overwritten.
       *  \param text                The text content of the header.
       */
      void fill_header(Gtk::TreeModel::Row &row,
                       Glib::ustring text) const;
  };

  /** \brief Interface for generating tree-views.
   *
   *  A tree-view generator takes each package that appears in the
   *  current package view and places it into an encapsulated
   *  Gtk::TreeModel.
   */
  class PackagesTreeModelGenerator
  {
  public:
    // FIXME: Hack while finding a nonblocking thread join.
    bool finished;
    virtual ~PackagesTreeModelGenerator();

    /** \brief Add the given package and version to this tree view.
     *
     *  \param pkg  The package to add.
     *  \param ver  The version of pkg to add, or an end
     *              iterator to add a versionless row.
     *
     *  \param reverse_package_store   A multimap in which a pair will
     *  be inserted for each row generated by this add().
     *
     *  \note Technically we could build reverse_package_store in a
     *  second pass instead of passing it here; I think this is
     *  cleaner and it might be worth giving it a try in the future.
     */
    virtual void add(const pkgCache::PkgIterator &pkg, const pkgCache::VerIterator &ver,
                     std::multimap<pkgCache::PkgIterator, Gtk::TreeModel::iterator> * reverse_package_store) = 0;

    /** \brief Perform actions that need to be taken after adding all
     *  the packages.
     *
     *  For instance, this typically sorts the entire view.
     */
    virtual void finish() = 0;

    /** \brief Retrieve the model associated with this generator.
     *
     *  The model will be filled in as add_package() is invoked.
     *  Normally you should only use the model once it is entirely
     *  filled in (to avoid unnecessary screen updates).
     *
     *  \return  The model built by this generator.
     */
    virtual Glib::RefPtr<Gtk::TreeModel> get_model() = 0;
  };

  class PackagesTreeView : public Gtk::TreeView
  {
    public:
      PackagesTreeView(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& refGlade);
      bool on_button_press_event(GdkEventButton* event);
      sigc::signal<void, GdkEventButton*> signal_context_menu;
      sigc::signal<void> signal_selection;
  };

  class PackagesView
  {
    public:
      typedef sigc::slot1<PackagesTreeModelGenerator *,
                          PackagesColumns *> GeneratorK;
    private:
      PackagesTreeView * treeview;
      PackagesColumns * packages_columns;
      std::multimap<pkgCache::PkgIterator, Gtk::TreeModel::iterator> * reverse_packages_store;
      PackagesContextMenu * context;
      PackagesMarker * marker;
      GeneratorK generatorK;
      void init(const GeneratorK &_generatorK,
                                   Glib::RefPtr<Gnome::Glade::Xml> refGlade);

      void on_cache_closed();
      void on_cache_reloaded();

      /** \brief Build the tree model using the given generator.
       *
       *  This adds all packages that pass the current limit to the
       *  generator, one at a time.
       *
       *  \param  generatorK         A function that constructs a generator
       *                             to use in building the new store.
       *  \param  packages_columns   The columns of the new store.
       *  \param  reverse_packages_store  A multimap to be filled with the
       *                                  location of each package iterator
       *                                  in the generated store.
       *  \param  limit             The limit pattern for the current view.
       */
      static Glib::RefPtr<Gtk::TreeModel> build_store(const GeneratorK &generatorK,
                                                      PackagesColumns *packages_columns,
                                                      std::multimap<pkgCache::PkgIterator, Gtk::TreeModel::iterator> * reverse_packages_store,
                                                      Glib::ustring limit);

      /** \brief Build the tree model using the given generator for a single package version.
       *
       *  This adds one package version to the generator.
       *
       *  \param  generatorK         A function that constructs a generator
       *                             to use in building the new store.
       *  \param  packages_columns   The columns of the new store.
       *  \param  reverse_packages_store  A multimap to be filled with the
       *                                  location of each package iterator
       *                                  in the generated store.
       *  \param  pkg                The package to add.
       *  \param  ver                The version to add.
       */
      static Glib::RefPtr<Gtk::TreeModel> build_store_single(const GeneratorK &generatorK,
                                                             PackagesColumns *packages_columns,
                                                             std::multimap<pkgCache::PkgIterator, Gtk::TreeModel::iterator> * reverse_packages_store,
                                                             pkgCache::PkgIterator pkg, pkgCache::VerIterator ver);
      Gtk::TreeViewColumn * CurrentStatus;
      Gtk::TreeViewColumn * SelectedStatus;
      Gtk::TreeViewColumn * Name;
      Gtk::TreeViewColumn * Section;
      Gtk::TreeViewColumn * Version;

      /** \brief Sets up generic column properties that don't have to do
       *  with creating the renderer.
       */
      void setup_column_properties(Gtk::TreeViewColumn *treeview_column,
				   int size);

      /** \brief Creates a column with a default renderer. */
      template <class ColumnType>
      int append_column(const Glib::ustring &title,
          Gtk::TreeViewColumn *treeview_column,
          Gtk::TreeModelColumn<ColumnType> &model_column,
          int size);

      /** \brief Creates a column that uses the given model column as
       *  Pango markup.
       */
      int append_markup_column(const Glib::ustring &title,
			       Gtk::TreeViewColumn *treeview_column,
			       Gtk::TreeModelColumn<Glib::ustring> &model_column,
			       int size);
    public:
      /** \brief Construct a new packages view.
       *
       *  The store will not be initialized if _limit is not set.
       *
       *  \param _generatorK A constructor for the callback
       *                     object used to build the model.
       *  \param refGlade    The XML tree containing
       *                     the widgets for this view.
       *  \param  limit      The limit pattern for the current view.
       */
      PackagesView(const GeneratorK &_generatorK,
                   Glib::RefPtr<Gnome::Glade::Xml> refGlade,
                   Glib::ustring limit = "");
    public:
      /** \brief Construct a new packages view.
       *
       *  \param _generatorK A constructor for the callback
       *                     object used to build the model.
       *  \param refGlade    The XML tree containing
       *                     the widgets for this view.
       *  \param  pkg        The package to add.
       *  \param  ver        The version to add.
       */
      PackagesView(const GeneratorK &_generatorK,
                   Glib::RefPtr<Gnome::Glade::Xml> refGlade,
                   pkgCache::PkgIterator pkg, pkgCache::VerIterator ver);
      ~PackagesView();
      void context_menu_handler(GdkEventButton * event);
      void row_activated_package_handler(const Gtk::TreeModel::Path &, Gtk::TreeViewColumn*);
      void refresh_packages_view(const std::set<pkgCache::PkgIterator> *changed_packages);
      void relimit_packages_view(Glib::ustring limit);
      // TODO: Not used yet, should be used in place of PackagesTab::activate_package_handler();
      //sigc::signal<void, pkgCache::PkgIterator, pkgCache::VerIterator> signal_on_package_selection;
      PackagesTreeView * get_treeview() const { return treeview; };
      PackagesColumns * get_packages_columns() const { return packages_columns; };
      PackagesMarker * get_marker() const { return marker; }
    Glib::RefPtr<Gtk::TreeModel> get_packages_store() const { return treeview->get_model(); };
      std::multimap<pkgCache::PkgIterator, Gtk::TreeModel::iterator> * get_reverse_packages_store() { return reverse_packages_store; };
  };

}

#endif /* PACKAGESVIEW_H_ */
