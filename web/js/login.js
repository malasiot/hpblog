$(function() {

	$("#login").click(function(e) {
		e.preventDefault() ;
		$("#login-modal").formModal('show', {url: 'user/login',  onSuccess: function() { location.reload(false); }}) ;
	}) ;

	$("#register").click(function(e) {
		e.preventDefault() ;
		$("#register-modal").formModal('show', {url: 'user/register',  onSuccess: function() { location.reload(false); }}) ;
	}) 

    $("#logout").click(function(e) {
	    e.preventDefault() ;
		$.ajax({
		    url: "user/logout/",
		    type: "POST",
			success: function(data)	{
       		   	document.location.href="";
            }
        }) ;
	}) ;
});

