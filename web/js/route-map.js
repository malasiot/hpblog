function OpenLayersMap (id, option)
{
    var projection3857 = ol.proj.get('EPSG:900913');
    var projection4326 = ol.proj.get('EPSG:4326');
    var options = option || {} ;
    
	var bingLayer = new ol.layer.Tile({
		title: 'Bing Aerial',
		type: 'base',
		visible: false,
		preload: Infinity,
		source: new ol.source.BingMaps({
			key: 'AjHrTkZRD_A50lN1GoMTCqZw67DtnYEx2a_uiVV4OAVG7Ii-_xNITLqN0uCwTUL7',
			imagerySet: 'AerialWithLabels',
			// use maxZoom 19 to see stretched tiles instead of the BingMaps
			// "no photos at this zoom level" tiles
			maxZoom: 19
		})
	});

    var osmLayer = new ol.layer.Tile({
    	title: 'OpenCycleMap',
    	type: 'base',
        source: new ol.source.OSM({
            attributions: [
            'All maps © <a href="https://www.opencyclemap.org/">OpenCycleMap</a>',
             ol.source.OSM.ATTRIBUTION
            ],
            url: 'https://{a-c}.tile.thunderforest.com/cycle/{z}/{x}/{y}.png' +
             '?apikey=ec23fa5af1a149cebc9eb1e57e3b0a23'
        })
    });
    
	var ktimaSource = new ol.source.TileWMS({
		url: 'https://gis.ktimanet.gr/wms/wmsopen/wmsserver.aspx',
		params: {'LAYERS': 'KTBASEMAP', 'VERSION': '1.1.1', 'TRANSPARENT': 'FALSE'}
	});
	
	var ktimaLayer = new ol.layer.Tile({
		title: "Κτηματολόγιο",
		type: "base",
        source: ktimaSource,
        visible: false
      });

    
    this.routesLayer =  new ol.layer.Tile({
    		title: 'Διαδρομές',
	    	source: new ol.source.XYZ({
              url: 'https://vision.iti.gr/mapcache/tms/1.0.0/routes/{z}/{x}/{-y}.png'
            })
	}) ;
    
    this.markers = new ol.layer.Vector({
        source: new ol.source.Vector({ features: [], projection: 'EPSG:4326' })
    });


    this.map = new ol.Map({
		layers: [ 
			new ol.layer.Group({
				title: 'Base Layer',
				layers: [osmLayer, bingLayer, ktimaLayer]
			}),
			new ol.layer.Group({
				title: "Overlays",
				layers: [ this.routesLayer]
			})
		],
		view: new ol.View({
			projection: projection3857,
			center: [2594832, 4867869],
			zoom: 7
		}),
		target: document.getElementById(id)
	});

 // Create a LayerSwitcher instance and add it to the map
    var layerSwitcher = new ol.control.LayerSwitcher();
	this.map.addControl(layerSwitcher);
   
    this.element = document.getElementById('popup') ;  
    
    $(this.element).popover({
		'placement': 'top',
    	'html': true,
    	'content': function() { 
    	  	return $('#popover-content').html() ;
		},
		'title': function() { 
    	  	return $('#popover-title').html() ;
		}
    }) ;   
   
    this.overlay = new ol.Overlay({
        element: this.element,
        positioning: 'bottom-center',
        stopEvent: false,
        autoPan: true,
        autoPanAnimation: {
            duration: 250
        }
    });
            
    this.map.addOverlay(this.overlay);
    
    this.wptLayer = null ;
        
    var that = this ;
    this.map.on('singleclick', function(evt) {
		var feature = that.map.forEachFeatureAtPixel(evt.pixel,  
			function(feature, layer) {
				if ( layer == that.wptLayer ) return feature;
				else return null ;
		});

		if ( feature ) {
			var geometry = feature.getGeometry();
			var coord = geometry.getCoordinates();
			that.overlay.setPosition(coord) ;
			var e = $(that.element) ;
			$('#popover-title').html(feature.get('name')) ;
			$('#popover-content').html(feature.get('desc')) ;
			e.popover('show');
		}
		else {
			$.ajax({
				type: "POST",
				dataType: "json",
				url: "query/route/",
				data: { 'x': evt.coordinate[0], 'y': evt.coordinate[1] },
				success: function(data) {
					if ( data ) {
						that.overlay.setPosition(evt.coordinate) ;
						var e = $(that.element) ;
						$('#popover-title').html('Διαδρομή') ;
						$('#popover-content').html('<a href="view/' + data.id + '">' + data.title + '</a>') ;
						e.popover('show');
					}
					else {
						that.overlay.setPosition(undefined);
						$(that.element).popover('hide');
					}
				}
	        }) ;
                    
		}
	});

    this.update = function(route_id, callback) {
        var that = this ;
        $.ajax({
            dataType: "json",
            url: "track/" + route_id + "/",
            success: function(data) {

                var box = data.box ;
        
                var ext = ol.proj.transformExtent(box, projection4326, projection3857);
                
                var format = new ol.format.GeoJSON({featureProjection: "EPSG:3857"});
                
                var tracks = format.readFeatures(data.tracks) ;
                
                var trackSource = new ol.source.Vector({features: tracks});
                
                that.trackLayer = new ol.layer.Vector({
                    source: trackSource,
                    style: new ol.style.Style({
                            stroke: new ol.style.Stroke({
                            color: 'red',
                            width: 3
                        })
                    })
                });
                    
                var wpts = format.readFeatures(data.wpts) ;
            
                var wptSource = new ol.source.Vector({features: wpts});
                
                var wptStyleFunc = function() {
				  	return function(feature, resolution) {
				  		var style =  new ol.style.Style({
	                        image: new ol.style.Circle({
    	                        radius: 4,
    	                        fill: new ol.style.Fill({color: 'yellow'}),
                                stroke: new ol.style.Stroke({color: 'red', width: 1})
    	                    }),
    	                    text: new ol.style.Text({
						        font: '12px Calibri,sans-serif',
    	    					fill: new ol.style.Fill({ color: '#000' }),
						        stroke: new ol.style.Stroke({
							        color: '#fff', width: 2
						        }),
						        // get the text from the feature - `this` is ol.Feature
						        // and show only under certain resolution
						        text: that.map.getView().getZoom() > 12 ? feature.get('name') : ''
		   					 })
    	                });
					    return [style];
  					};
				};
				
                that.wptLayer = new ol.layer.Vector({
                    source: wptSource,
                    style: wptStyleFunc()
                });
                
                that.map.addLayer(that.trackLayer) ;
                that.map.addLayer(that.wptLayer) ;
                that.map.getView().fit(ext, that.map.getSize());
                
                if ( callback !== undefined ) 
                	callback() ;
            }
        });
    }
       
    this.setMarker = function(lat, lon) {
    	var coords = ol.proj.transform([lon, lat], 'EPSG:4326', 'EPSG:3857') ;
		var geom = new ol.geom.Point(coords);
	   	var feature = new ol.Feature(geom);
		feature.setStyle([
			new ol.style.Style({
         		image: new ol.style.Icon(({
                 	anchor: [0, 0],
                 	anchorXUnits: 'fraction',
                	anchorYUnits: 'fraction',
                 	opacity: 1,
                 	src: 'img/pin.png'
       			}))
      		})
   		]);

	
   		var src = this.markers.getSource() ;
   		src.clear() ;
   		src.addFeature(feature);
   		this.map.getView().setCenter(coords) ;
    }
}

