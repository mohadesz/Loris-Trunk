function getQueryVariable(variable) {
	var query = window.location.search.substring(1);
	var vars = query.split("&");
	for (var i = 0; i < vars.length; i++) {
		var pair = vars[i].split("=");
		if (pair[0] == variable) {
			return unescape(pair[1]);
		}
	}
}

function changefieldOptions() {
	changeVisitLabels();
}

function changeVisitLabels() {
    "use strict";
    //get the value for the visit selected
    var instrument_dropdown = document.getElementById('instrument'),
        visit_label_dropdown = document.getElementById('visit_label'),
        visit_label_value = visit_label_dropdown.value,
        request = getQueryVariable("visit_label"),
        instrument_dropdown_value,
        temp_array = ["All Instruments"],
        instruments;
    if ($.trim(visit_label_value) === 'All Visits') {visit_label_value = ''; }
    if (request !== undefined) {
        request = request.replace(/\+/g, ' ');
    }
    instrument_dropdown_value = getQueryVariable("instrument");
    if (instrument_dropdown_value !== undefined) {
        instrument_dropdown_value = instrument_dropdown_value.replace(/\+/g, ' ');
    }
    $.get("AjaxHelper.php?Module=data_team_helper&script=GetInstruments.php&visit_label=" + visit_label_value,
        function (data) {
            instruments = data.split("\n");
            instruments = temp_array.concat(instruments); //adds 'All instruments to the array'
            instrument_dropdown.options.length = 0;
            var i, numInstruments = instruments.length, val;
            for (i = 0; i < numInstruments; i += 1) {
                val = instruments[i];
                if (val !== '') {
                    instrument_dropdown.options[i] = new Option(val, val);
                    if ((instrument_dropdown_value === val) && (instrument_dropdown_value !== '')) {
                        instrument_dropdown.options[i].selected = "selected";
                    }
                }
            }
            //jQuery('#visits').change();
        });
}


//runs the function when the page is loaded..
$(function(){
	
	changefieldOptions();

	$('#instrument').bind('change',function(event){$("#filter").trigger('click');}); //The form is automatically loaded when the dropdown is changed

	 save();
	});
	
	
	
function save() {
    "use strict";
    var default_value, id, value;
    /**To get the default value**/
    
    $('.comment').blur(function (event) {
         id = event.target.id;
         value = $("#" + id).text();
         $.get("UpdateDiComments.php?id=" + id + "&value=" + value , function () {});
    }).keypress(function (e) {
        if (e.which === 13) { // Determine if the user pressed the enter button
            $(this).blur();
        }
})};
