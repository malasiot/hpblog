#include "forms.hpp"

#include <twig/renderer.hpp>
#include <ws/crypto.hpp>

using namespace std ;
using namespace twig ;
using namespace ws ;

void FormField::fillData(Variant::Object &res) const {

    if ( !name_.empty() ) res.insert({"name", name_}) ;

    if ( !value_.empty() ) res.insert({"value", value_}) ;
    else if ( !initial_value_.empty() ) res.insert({"value", initial_value_}) ;

    if ( !errors_.empty() ) res.insert({"errors", Variant::fromVector(errors_)}) ;
}

bool FormField::validate(const string &value)
{
    // normalize passed value

    string n_value = normalizer_ ? normalizer_(value) : value ;

    // call validators

    for( auto &v: validators_ ) {
        try {
            v->validate(n_value, *this) ;
        }
        catch ( FormFieldValidationError &e ) {
            addErrorMsg(e.what());
            return false ;
        }
    }

    // store value

    value_ = n_value ;
    is_valid_ = true ;
    return true ;
}

FormHandler::FormHandler() {}

void FormHandler::addField(const FormField::Ptr &field) {
    fields_.push_back(field) ;
    field_map_.insert({field->getName(), field}) ;
}


Variant::Object FormHandler::view() const
{
    Variant::Object form ;

    for( const auto &p: fields_ ) {
      if ( !p->value_.empty() )
            form.insert({p->name_, p->value_}) ;
    }

    return form ;
}

Variant::Object FormHandler::errors() const
{
    Variant::Object e, fields ;

    e.insert({"global_errors", Variant::fromVector(errors_)}) ;
    for( const auto &f: fields_ ) {
        fields.insert({f->name_, Variant::fromVector(f->errors_)}) ;
    }
    e.insert({"field_errors", fields}) ;

    return e ;
}

string FormHandler::getValue(const string &field_name)
{
    const auto &it = field_map_.find(field_name) ;
    if ( it != field_map_.end() ) return it->second->value_ ;
    else return string() ;
}


void FormHandler::handle(const Request &request, Response &response, TemplateRenderer &engine)
{
    if ( request.getMethod() == "POST" ) {

        if ( validate(request) ) {
            onSuccess(request) ;

            // send a success message
            response.writeJSON(Variant(Variant::Object{{"success", true}}).toJSON()) ;
        }
        else {
            response.writeJSON(Variant(Variant::Object{{"success", false}, {"errors", errors() }}).toJSON());
        }
    }
    else {
        onGet(request) ;
        response.writeJSON(Variant(view()).toJSON());
    }
}

bool FormHandler::validate(const Request &req) {
    bool failed = false ;

    // validate POST params

    for( const auto &p: fields_ ) {
        string v = req.getPostAttribute(p->name_) ;
        if ( !v.empty() ) {

            bool res = p->validate(v) ;
            if ( !res ) failed = true ;
        }
    }

    // validate file params
    // in this case the validator only receives the name of the field in the FILE dictionary
    // and is responsible for fetchinf the data from there

    for( const auto &p: fields_ ) {
        auto it = req.getUploadedFiles().find(p->name_) ;
        if ( it != req.getUploadedFiles().end() ) {
            bool res = p->validate(p->name_) ;
            if ( !res ) failed = true ;
        }
    }

    is_valid_ = !failed ;
    return is_valid_ ;
}

void FormHandler::init(const Dictionary &vals) {
    for( const auto &p: vals ) {
        auto it = field_map_.find(p.first) ;
        if ( it != field_map_.end() ) {
            it->second->value(p.second) ;
        }
    }
}

