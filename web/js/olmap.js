

function OpenLayersMap (id, option) 
{
	var options = option || {} ;
	var smercProj = new ol.Projection("EPSG:900913") ; 
	var wgs84Proj = new ol.Projection("EPSG:4326") ;
		
	var map_state_cookie = "mapstate" ;
	var map_state_cookie_expiredays = 7; 
	var map_features_cookie = "mapfeatures" ;
	var map_features_cookie_expiredays = 1 ;
	
	var bounds = options.bounds ? new ol.Bounds(options.bounds).transform(wgs84Proj, smercProj) : null ;
	
	var navControl = new ol.Control.Navigation() ;
	
	this.map = new ol.Map (id, {
		controls:[
			navControl,
			new OpenLayers.Control.Zoom(),
			new OpenLayers.Control.Permalink(),
			new OpenLayers.Control.ScaleLine({geodesic: true}),
			new OpenLayers.Control.Permalink('permalink'),
			new OpenLayers.Control.MousePosition(),                    
			new OpenLayers.Control.Attribution(),
			new OpenLayers.Control.LayerSwitcher()
		],
	    units: 'm',
        projection: smercProj,
        displayProjection: wgs84Proj,
        numZoomLevels: 22
  
	} );
	
	if ( bounds ) this.map.mapExtent = bounds ;
	
	var that = this ;
	this.map.events.on({"moveend": function(e) { 
		that.saveMapState(); 
	}, "zoomend": function(e) { 
		that.saveMapState() ;
	}, scope: this.map}); 

	

	OpenLayers.Control.Click = OpenLayers.Class(OpenLayers.Control, {                
        defaultHandlerOptions: {
			'single': true,
            'double': false,
            'pixelTolerance': 0,
            'stopSingle': false,
            'stopDouble': false
        },

        initialize: function(options) {
			this.handlerOptions = OpenLayers.Util.extend(
                        {}, this.defaultHandlerOptions
            );
            OpenLayers.Control.prototype.initialize.apply(
                        this, arguments
            ); 
            this.handler = new OpenLayers.Handler.Click(
						this, {
                            'click': this.trigger
                        }, this.handlerOptions
            );
        }, 

        trigger: function(e) {
            var lonlat = this.map.getLonLatFromViewPortPx(e.xy) ;
								
			params = "?lat=" +  lonlat.lat + "&lon=" + lonlat.lon + "&map=" + this.mapid ;
					
			OpenLayers.Request.GET({ url: this.url + params, scope: this, callback:
						function(response)
						{
							this.map.addPopup(new OpenLayers.Popup.FramedCloud(
							"feature", 
							lonlat,
							new OpenLayers.Size(400, 100), 
							response.responseText,
							null,
							true
							));
						}
			});
                    
        }

	});

	this.addClickHandler = function(url, id)
	{
		var click = new OpenLayers.Control.Click({url: url, mapid: id});
		this.map.addControl(click);
		click.activate();
	}
				
	this.initMapState = function() {
	
	//	this.restoreMapFeatures() ;
		
		// === Look for the cookie ===
	
		if ( this.cookies && document.cookie.length > 0 ) 
		{
			cookieStart = document.cookie.indexOf(map_state_cookie + "=");
			
			if (cookieStart !=-1 ) 
			{
				cookieStart += map_state_cookie.length + 1; 
				cookieEnd = document.cookie.indexOf(";",cookieStart);
          
				if ( cookieEnd==-1 ) cookieEnd=document.cookie.length;
          
				cookietext = document.cookie.substring(cookieStart,cookieEnd);
				// == split the cookie text and create the variables ==
				bits = cookietext.split("|");
				lat = parseFloat(bits[0]);
				lon = parseFloat(bits[1]);
				zoom = parseInt(bits[2]);
				layer = parseInt(bits[3]);
				this.map.setCenter(new OpenLayers.LonLat(lon, lat), zoom);
				this.map.setBaseLayer(this.map.layers[layer]) ;
				return ;

			}	
	
		}
		
		if ( !bounds ) this.map.zoomToMaxExtent();
		else this.map.zoomToExtent(bounds, false) ;
	}

	this.restoreMapFeatures = function() 	{
	
		// === Look for the cookie ===
		if ( document.cookie.length > 0 ) 
		{
			cookieStart = document.cookie.indexOf(map_features_cookie + "=");
			
			if (cookieStart !=-1 ) 
			{
				cookieStart += map_features_cookie.length + 1; 
				cookieEnd = document.cookie.indexOf(";",cookieStart);
          
				if ( cookieEnd==-1 ) cookieEnd=document.cookie.length;
          
				cookietext = document.cookie.substring(cookieStart,cookieEnd);
				bits = cookietext.split("|");
				wpts = bits[0] ;
				tracks = bits[1] ;
				
				importer = new OpenLayers.Format.GeoJSON() ;
				vlayer_wpts.addFeatures(importer.read(wpts)) ;
				vlayer_tracks.addFeatures(importer.read(tracks)) ;
				
				return ;

			}	
	
		}
		
	}
	
	this.saveMapState = function() 	{
	
		mapcenter = new OpenLayers.LonLat(this.map.getCenter().lon, this.map.getCenter().lat);
        var cookietext = map_state_cookie + "=" + mapcenter.lat + "|" + mapcenter.lon + "|" + this.map.getZoom() +  "|" + this.map.layers.indexOf(this.map.baseLayer);
		
        if ( map_state_cookie_expiredays ) 
		{
			var exdate = new Date();
			exdate.setDate(exdate.getDate() + map_state_cookie_expiredays );
			cookietext += ";expires=" + exdate.toGMTString();
        }
        // == write the cookie ==
        document.cookie = cookietext;
		
	//	this.saveMapFeatures() ;
    }
	
	this.saveMapFeatures = function()	{
		
		exporter = new OpenLayers.Format.GeoJSON() ;
		var cookietext = map_features_cookie + "=" + exporter.write(vlayer_wpts.features) + "|" +  exporter.write(vlayer_tracks.features) ;
		
        if ( map_features_cookie_expiredays ) 
		{
			var exdate = new Date();
			exdate.setDate(exdate.getDate() + map_features_cookie_expiredays );
			cookietext += ";expires=" + exdate.toGMTString();
        }
	
        // == write the cookie ==
        document.cookie = cookietext;
				
    }

	var counter = 1 ;
	
	this.addWaypoint = function(evt) {
		
		evt.feature.attributes.label = counter ;
		if ( evt.type == "beforefeatureadded" ) counter ++ ;
		return true ;
		
	}

	DeleteFeature = OpenLayers.Class(OpenLayers.Control, {
    	initialize: function(layer, options) {
        	OpenLayers.Control.prototype.initialize.apply(this, [options]);
        	this.layer = layer;
        	this.handler = new OpenLayers.Handler.Feature(
        	    this, layer, {click: this.clickFeature}
        	);
	    },
	    clickFeature: function(feature) {
                 this.layer.destroyFeatures([feature]);
         },
    	setMap: function(map) {
    	    this.handler.setMap(map);
    	    OpenLayers.Control.prototype.setMap.apply(this, arguments);
    	},
    	CLASS_NAME: "OpenLayers.Control.DeleteFeature"
	});


	this.addGPXOverlay = function(fileName, url) {
		
	// Add the Layer with the GPX Track

		var gpxStyles = new OpenLayers.StyleMap(
			{ "default": new OpenLayers.Style({
					label:  "${name}",
					labelAlign: 'cb',
					fontSize: 9,
					fontFamily: "Arial",
					fontColor: "red",
					labelYOffset: 6,
					fillColor: "brown",
					strokeColor: "yellow",
					strokeWidth: 2,
					strokeOpacity: 1
				},
			    {
					rules: [
						new OpenLayers.Rule({
							minScaleDenominator: 20000,
							symbolizer: {
								pointRadius: 0,
								fontSize: "0px"
							}
						}),
						new OpenLayers.Rule({
							maxScaleDenominator: 20000,
							minScaleDenominator: 10000,
							symbolizer: {
								pointRadius: 4,
								fontSize: "8px"
							}
						}),
						new OpenLayers.Rule({
							maxScaleDenominator: 10000,
							symbolizer: {
								pointRadius: 8,
								fontSize: "10px",
								labelYOffset: 10
							}
						})
					]
				}),
			"select": new OpenLayers.Style({fillColor: "#66ccff", 
                             strokeColor: "#3399ff",graphicZIndex: 2
			})
		});
		
		var lgpx = new OpenLayers.Layer.Vector(fileName, {
			strategies: [new OpenLayers.Strategy.Fixed()],
			protocol: new OpenLayers.Protocol.HTTP({
				url: url,
				format: new OpenLayers.Format.GPX({extractWaypoints:  true, extractRoutes: true, 
                  extractAttributes: true})
			}),
			styleMap: gpxStyles,
			projection: new OpenLayers.Projection("EPSG:4326")
		});
		this.map.addLayer(lgpx);
		
		var that = this ;
		var selectControl, selectedFeature ;
		
		function onPopupClose(evt) {
            selectControl.unselect(selectedFeature);
        }
        
        function onFeatureSelect(feature) {
            selectedFeature = feature;
            
            var pixel = selectControl.handlers.feature.evt.xy;
			var location = that.map.getLonLatFromPixel(pixel);
    
            popup = new OpenLayers.Popup.FramedCloud("chicken", 
                                     location, //( feature.geometry.CLASS_NAME == "OpenLayers.Geometry.Point" ) ? feature.geometry.getBounds().getCenterLonLat(): eature.geometry.getBounds().getCenterLonLat(),
                                     null,
                                     "<div style='font-size:.8em'>" + feature.attributes.desc + "</div>",
                                     null, true, onPopupClose);
            feature.popup = popup;
            that.map.addPopup(popup);
        }
        
        function onFeatureUnselect(feature) {
            that.map.removePopup(feature.popup);
            feature.popup.destroy();
            feature.popup = null;
        }    
		
		selectControl = new OpenLayers.Control.SelectFeature(
			lgpx,
			{
				onSelect: onFeatureSelect,
				onUnselect: onFeatureUnselect,
				autoActivate: true
			}
		);
		this.map.addControl(selectControl);
		
	}

	this.saveFeatures = function()
	{	
		
        if ( map_features_cookie_expiredays ) 
		{
			var exdate = new Date();
			exdate.setDate(exdate.getDate() + map_features_cookie_expiredays );
			cookietext += ";expires=" + exdate.toGMTString();
        }
		//alert(cookietext.length) ;
        // == write the cookie ==
        document.cookie = cookietext;
				
	}
	

	this.showSources = function()
	{
		$('#copyright').jqmShow() ;
	}
	
	// Google maps api should be included for this to work
	
	this.addGoogleMapsLayerTerrain = function(label) {
		
		label = label || "Google Physical" ;
		
		var layer = new OpenLayers.Layer.Google(
                label,
                {type: google.maps.MapTypeId.TERRAIN}
            ) ;
            
         this.map.addLayer(layer) ;
	 }
	 
	 this.addGoogleMapsLayerStreets = function(label) {
		
		label = label || "Google Streets" ;
		
		var layer = new OpenLayers.Layer.Google(
                label
            ) ;
            
         this.map.addLayer(layer) ;
	 }
	 
	 this.addGoogleMapsLayerHybrid = function(label) {
		
		label = label || "Google Hybrid" ;
		
		var layer = new OpenLayers.Layer.Google(
                label,
                {type: google.maps.MapTypeId.HYBRID, numZoomLevels: 20}
            ) ;
            
         this.map.addLayer(layer) ;
	 }
	 
	 this.addGoogleMapsLayerSatellite = function(label) {
		
		label = label || "Google Satellite" ;
		
		var layer = new OpenLayers.Layer.Google(
                label,
                {type: google.maps.MapTypeId.SATELLITE, numZoomLevels: 22}
            ) ;
            
         this.map.addLayer(layer) ;
	 }
	 
	 this.addKtimatologioLayer = function(label) {
		
		label = label || "Ktimatologio S.A." ;
		
		layer = new OpenLayers.Layer.WMS( label,
                    "http://gis.ktimanet.gr/wms/wmsopen/wmsserver.aspx?",
                    {
						layers: 'KTBASEMAP',
						numZoomLevels: 22
					});
		 
		this.map.addLayer(layer) ;
	 }
	 
	 this.addOpenCycleMapLayer = function(label) {
		
		label = label || "OpenCycleMap" ;
		
		layer = new OpenLayers.Layer.OSM("OpenCycleMap",
			["http://a.tile.thunderforest.com/cycle/${z}/${x}/${y}.png",
			"http://b.tile.thunderforest.com/cycle/${z}/${x}/${y}.png",
			"http://c.tile.thunderforest.com/cycle/${z}/${x}/${y}.png"]);
		 
		this.map.addLayer(layer) ;
	 }

	function get_url (bounds) {
	        var res = this.map.getResolution();
	        var x = Math.round ((bounds.left - this.maxExtent.left) / (res * this.tileSize.w));
        	var y = Math.round ((this.maxExtent.top - bounds.top) / (res * this.tileSize.h));
	        var z = this.map.getZoom();
        	
	        var url = this.url + "&x=" + x + "&y=" + y + "&z=" + z ;
	        return url ;
  	}

 	this.addOverlayLayer = function(label, id) {

		label = label || "Overlay" ;
		
		layer = new OpenLayers.Layer.TMS(label, "http://vision.iti.gr:5454/?map=" + id, { 'type':'png', 'getURL': get_url, isBaseLayer: false });
		 
		this.map.addLayer(layer) ;
	}


    this.addKMLOverlay = function(label, url, maxscale) {

		label = label || "KML" ;
		maxscale = maxscale || this.map.maxScale ;
		
		  layer = new OpenLayers.Layer.Vector(label, {
		  maxScale: maxscale,
		  isBaseLayer: false,
            strategies: [new OpenLayers.Strategy.Fixed()],
            projection: this.map.displayProjection,
            protocol: new OpenLayers.Protocol.HTTP({
                url: url,
                format: new OpenLayers.Format.KML({
                    extractStyles: true,
                      extractAttributes: true,
                    maxDepth: 2
                })
            })
        })
		
		layer.setVisibility(true) ;
		this.map.addLayer(layer) ;
	}

	    
/*
   var lonlat = olmap.getLonLatFromViewPortPx(e.xy) ;
			var lonlatwgs84 = lonlat.clone() ;
			lonlatwgs84.transform(olmap.projection, wgs84Proj) ; 
					
			uri = "http://vision.iti.gr/routes/featureinfo.php" ;
			params = "?lat=" +  lonlat.lat + "&lon=" + lonlat.lon + "&scale=" + olmap.getScale() ;
*/

	this.init = function()
	{

		this.initMapState() ;
	}

	this.overlay_getTileURL = function(bounds) {

		var mapBounds = new OpenLayers.Bounds(-20037508, -20037508, 20037508, 20037508.34)
		var mapMinZoom = 10 ;
		var mapMaxZoom = 18 ;
	
		var res = this.map.getResolution();
		var x = Math.round((bounds.left - mapBounds.left) / (res * this.tileSize.w));
		var y = Math.round((bounds.bottom - this.tileOrigin.lat) / (res * this.tileSize.h));
		var z = this.map.getZoom();
  	
  	
		if (mapBounds.intersectsBounds( bounds ) && z >= mapMinZoom && z <= mapMaxZoom ) 
		{
	               //console.log( this.url + z + "/" + x + "/" + y + "." + this.type);
			return this.url + z + "/" + x + "/" + y + "." + this.type;
		} else {
			return "http://www.maptiler.org/img/none.png";
		}
	}		
  
}
