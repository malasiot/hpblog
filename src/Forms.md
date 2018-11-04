# Form Helpers

## Defining a new form

The Form class provides mechanisms for form rendering, validation and request handling. 
The normal usage pattern is to define a new class inheriting from Form which will also encapsulate
context parameters such as database connection, user authentication etc.
```C++
class MyForm: public wspp::web::Form {
public:
    MyForm(Connection &con, MyModel &model) ;

    void onSuccess(const Request &request) override;
    void onGet(const Request &request) override;
private:

    Connection &con_ ;
    MyModel &model_ ;
};
```
In the constructor one should then define the form fields by calling the `addField()` function or more conveniently `field<T>()` function with the appropriate field type `T`. 
```C++
  field<InputField>("title", "text").label("Title").required()
        .addValidator<NonEmptyValidator>() ;
```
All field types inherit from "FormField" class which acts as a container of form field parameters. These parameters are then used for rendering, validation etc. One may define custom form fields accordingly by overriding "fillData" function of the FormField class to populate a variant Object with the appropriate key/value pairs. Fields are indexed by their name which should be unique. 

## Validation
Each field can have zero or more validators. A validator is a functor that inherits from "FormFieldValidator" class and overrides 
```C++
  virtual void validate(const string &val, const FormField &field) const = 0 
```
This takes as input the passed request value, checks for its correctness, and in case of error throw a "FormFieldValidationError" exception with appropriate message string. For making custom error messages each validator constructor takes as input a message template. Simple string subsititution is used to replace "{variable}" instances in the message using the function:
```C++
string FormFieldValidator::interpolateMessage(const string &msg_template, const string &value, const FormField &field, const Dictionary &params= Dictionary()) ;
```
The special variables "{field}" and "{value}" are substituted with the field label and value respectively. Other variables are looked up in the `params` dictionary.   

Some predefined validators are `NonEmptyValidator` (checks that the passed value is not empty), the `MinLengthValidator` and `MaxLengthValidator` (check for min and max number characters) and `RegexValidator` (checks if the input value matches a custom Regular expression).

Custom validators may be also defined using a lambda.
```C++
 field<InputField>("username", "text").required().label("Username")
        .addValidator<NonEmptyValidator>()
        .addValidator([&] (const string &val, const FormField &f) {
            if ( user_.userNameExists(val) )
                throw FormFieldValidationError("username already exists") ;
        }) ;
```
where the `user_` has been captured as a member of the containing form.

When the form `validate()` function is called, it calls in turn the `validate()` function for each field, any exceptions are caught and corresponding validation messages are recorded on a dictionary which may be then obtained using the `errors()` function. If no errors occur then `validate()` will return true and will also store the passed value for the field.

## Handling form requests

Form class provides a helper function to facilitate request handling (i.e. form submission) using server-side form rendering.
```C++
void Form::handle(const Request &request, Response &response, TemplateRenderer &engine)
```
If this is a "GET" request it renders the form using the provided engine instance. It will first call the `onGet()` function which may be overriden to initialise the form e.g. from values received from the database. Otherwise if it is a "POST" request, it will try to validate the form. If the validation is successful `onSuccess()` will be called first which may perform persistence actions. A JSON response `{"success": true}` is returned. If the submitted form is invalid then it will be rendered with error messages and the corresponding payload will be returned with the JSON key "content" while "success" will be set to "false". 

## Form rendering

This is performed by means of the `render()` function. The function will populate a Variant::Object with the form parameters and will then render a provided template using the passed rendering engine. The key/values passed to the template renderer (if defined) are:
- enctype
- action
- method
- button.name
- button.title
- global_errors: [],  list of message strings
- fields: [], list of fields

For each field:
- label, name, id, placeholder, help_text, required, disabled, extra_classes, extra_attrs
- value: <field_value>, initial value if error in validation otherwise submitted value
- error_messages: [], list of message strings corresponding to validation errors
- Other field specific parameters e.g. type for input fields    
