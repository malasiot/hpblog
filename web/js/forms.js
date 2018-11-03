// helper for auto-handling of form submit and form dialogs

(function($) {
	$.fn.form = function(params) { 
		var defaults = { data: {}, onSuccess: function() {} } ;
		var params = $.extend( {}, defaults, params );
	
		// traverse all nodes
		this.each(function() {
			var that = $(this);
			var form = that.find('form') ;
			
			if ( params.values ) {
				for( var key in params.values ) {
					form.find( "[name*='" + key + "']" ).val( params.values[key] );
				}
			}
			
			
			function clearErrors() {
				$('#global-errors', form).remove() ;
				
				$('.form-group' , form).each(function(index) {
					$(this).removeClass("has-error has-feedback") ;
					$('#errors', this).remove() ;
				}) ;
			}
			
			form.submit(function(e) {
				var form_data = new FormData(this);
				for ( var key in params.data ) {
    				form_data.append(key, params.data[key]);
				}	
				e.preventDefault() ;

				$.ajax({
					type: "POST",
					dataType: "json",
					url: params.url,
					processData: false,
				   	contentType: false,
//					data: form.serialize(), // serializes the form's elements.
					data: form_data,
                    error: function(jqXHR, textStatus, errorThrown) {
                        console.log(jqXHR.status);
                    },
                    success: function(data)	{
						if ( data.success ) 
							params.onSuccess() ;
						else {
							if ( data.hasOwnProperty("content") ) // server has send the whole form
								form.html(data.content) ;
							else { // render errors	only
								clearErrors() ;
								var errors = data.errors ;
								if ( errors.global_errors.length > 0 ) {
									var et = $('<div id="global-errors" class="alert alert-danger"><a class="close" data-dismiss="alert">&times;</a></div>') ;
									et.prependTo(form);
									for( item in errors.global_errors ) {
										et.append('<p>' + errors.global_errors[item] + '</p>') ;
    	    						}
								}
								for( key in errors.field_errors ) {
									var error_list = errors.field_errors[key] ;
									if ( error_list.length > 0 ) {
										var et = $('.form-group [name=' + key + ']', form).parent() ;
										et.addClass("has-error has-feedback") ;
										var error_block = $('<div id="errors"></div>') ;
	
										$('<span class="glyphicon glyphicon-remove form-control-feedback"></span>').appendTo(error_block) ;
										var msg_block = $('<span class="help-block"></span>') ;
										for( item in error_list ) {
											msg_block.append('<p>' + error_list[item] + '</p>') ;
    		    						}
    		    						msg_block.appendTo(error_block) ;
									    error_block.appendTo(et) ;
									}
								}
							}
							
						}
					} 
				});
								
				
			}) ;
			
	
		});
		
		return this ;
	};

	$.fn.formModal = function(cmd, params) { 
	
		var defaults = { onSuccess: function () {}, data: {} } ;
		params = $.extend( {}, defaults, params );
		
		var that = this ;
		
		function load(data_values) {
	
			values = $.extend( {}, {}, data_values );
			var content = that.find('.modal-body') ;
			
            var form = content.form({
                url: params.url,
                data: params.data,
                values: values,
                onSuccess: function() {
                    params.onSuccess() ;
                    that.modal('hide') ;
                }
            }) ;
            that.modal('show') ;
			
		} ;
		
		this.each(function() {
            if ( cmd === 'show' ) {
    			if ( params.init ) {
					$.ajax({
						type: "GET",
						dataType: "json",
						data: params.data,
						url: params.url,
            	        error: function(jqXHR, textStatus, errorThrown) {
            	            console.log(jqXHR.status);
            	        },
            	        success: function(data)	{
							if ( data ) 
								load(data) ;
						}
					}) ;
				}
				else
					load() ;
			}	
		});
		
		return this ;
	};
	

})(jQuery);
