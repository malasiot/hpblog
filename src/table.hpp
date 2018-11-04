#ifndef APP_UTIL_TABLE_VIEW_HPP
#define APP_UTIL_TABLE_VIEW_HPP

#include <string>
#include <vector>

#include <twig/variant.hpp>
#include <ws/request.hpp>
#include <ws/response.hpp>
#include <xdb/connection.hpp>

// abstraction of a table view

class TableView {
public:

    TableView() {}

    // return total records
    virtual uint count() = 0 ;

    // return count records starting at offset
    virtual twig::Variant rows(uint offset, uint count) = 0 ;

    // fetch data to pass to the template renderer
    twig::Variant::Object fetch(uint page, uint results_per_page);

    // a hook to modify the display value of a cell
    virtual twig::Variant transform(const std::string &key, const std::string &value) { return value ; }

    // render the table
    void handle(const ws::Request &request, ws::Response &response) ;

};

// Table view based on an SQlite database table

class SQLTableView: public TableView {
public:

    // Each row is populated by performing a select on the table, recovering the column names and associates values
    // For complex tables (e.g. with joins) or you want to rename the column names, subclass and create an SQLite view in the constructor, passing the name of
    // the view to the base class instead of the original table. Note that the id should always be returned.
    SQLTableView(xdb::Connection &con, const std::string &table, const std::string &id_column = "id");

    twig::Variant rows(uint offset, uint count) override;

    uint count() override {
        return con_.query("SELECT count(*) FROM " + table_).getOne()[0].as<uint>() ;
    }

protected:
    xdb::Connection &con_ ;
    std::string table_, id_column_ ;
};






#endif
