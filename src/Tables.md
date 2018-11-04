# Tables

The class `TableView` is an abstract interface for managing rendering of tables with paging functionality. Most usefull is `SQLTableView` which implements the interface for an SQL table. 

For example consider a table `attachments` that stores links to files attached to a page. The table has the fields: id, page_id, name, url, type. The type field is an integer that can take one of three possible values. When we want to show the page attachments, we should have a table with two columns, file name rendered as a link, and type renderer as a string from the closed set (i.e. "Image", "PDF file", "Video"). This can be implemented with the following code.

```C++
class AttachmentTableView: public SQLTableView {
public:
    AttachmentTableView(Connection &con, const std::string &page_id, const Dictionary &amap):
        SQLTableView(con, "attachments_list_view"), attachments_map_(amap) {

        setTitle("Attachments") ;

        string sql("CREATE TEMPORARY VIEW attachments_list_view AS SELECT id, name, url, type FROM attachments WHERE page_id = ") ;
        sql += route_id;

        con_.execute(sql) ;

        addColumn("Name", "<a href=\"{{url}}\">{{name}}</a>") ;
        addColumn("Type", "{{type}}") ;
    }

    Variant transform(const string &key, const string &value) override {
        if ( key == "type" ) {
            auto it = attachments_map_.find(value) ;
            if ( it != attachments_map_.end() ) return it->second ;
            else return string() ;
        }
        return value ;
    }
private:
    Dictionary attachments_map_ ;
};
```
Note that we created a view to request only the attachments of a specific page. An SQL view is also usefull when we need joins with other tables. Internally the query will be further limited by appending `LIMIT` to achieve paging.

The `transform` function offers a hook to alter the data before being send to the template renderer. In the above case a dictionary is used to map a `type` identifier to a corresponding label but other complex transformations may be performed and also returning complex data models. 

We may use a `widget` parameter passed to each column, which is mustache template, to alter the display of a column.

Once an instance of a TableView is constructed we may call the render function which will produce the data model to be passed to the template renderer.

- title
- page
- total_pages
- total_rows
- headers: [{name: xxx, widget: xxx}, ...]
- rows: [{id: xxx, data: {...}}, ...]
- pager: {page: xxx, text: xxx, first: xxx, previous: xxx, next: xxx, last: ..., active: xxx, disabled: xxx} 

