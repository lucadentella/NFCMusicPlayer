$(document).ready(function(){

// get the list of songs in the SD card
$.getJSON("/songs", function(data) {	
	$.each(data, function(key, val) {
		$('#songs-select').append(new Option(val, val));
	});
});

// get the current list of mappings
$.getJSON("/mappings", function(data) {	
	$.each(data, function(key, val) {
		newElement(val.tagid, val.song);
	});
});

// create a new list item
function newElement(tagid, song) {
	
	// <li> element
	var li = document.createElement("li");
	li.setAttribute("tagid", tagid);
	var t = document.createTextNode(tagid + " - " + song);
	li.appendChild(t);
	document.getElementById("myUL").appendChild(li);

	// close button
	var span = document.createElement("SPAN");
	var txt = document.createTextNode("\u00D7");
	span.className = "close";
	span.appendChild(txt);
	li.appendChild(span);
	span.onclick = function() {
		removeElement(this.parentElement);
	}
}

function removeElement(li) {
	
	tagid = li.getAttribute("tagid");
	console.log("Will remove " + tagid);
	$.ajax({
		type: 'POST',
		url: '/delmapping',
		data: JSON.stringify ({tagid: tagid}),
		contentType: "application/json",
		dataType: 'json',
		complete: function(jqXHR, textStatus) {
			if(jqXHR.status == 200) li.remove();
			else alert('Unable to remove mapping: ' + jqXHR.responseJSON["result"]);
		}
	});
}

// Add onclick events to buttons

$("#get-tag-id-button").on("click", function() {
	
	// get current TAG ID
	$.get("/tagid", function(data) {
		$("#tag-id").val(data.tagid);
	});
});


$("#add-mapping-button").on("click", function() {
	
	// get the new tagid and song name
	newtagid = $("#tag-id").val();
	newsong = $("#songs-select").find(":selected").text();
	
	// check if newtagid is valid
	if(newtagid == "") {
		alert("No Tag ID found, did you press the Get Tag ID button with a Tag on the player?");
		return;
	}
		
	
	// check if tagid is already mapped
	found = false;
	$('li').each(function() {
		tagid = this.getAttribute("tagid");
		if(newtagid == tagid) {
			alert("Tag ID already mapped!");
			found = true;
		}
	});
	
	if(!found) {
		
		// save the new mapping
		console.log("Adding " + newtagid + " - " + newsong);
		
		// make the REST call
		$.ajax({
			type: 'POST',
			url: '/addmapping',
			data: JSON.stringify ({tagid: newtagid, song: newsong}),
			contentType: "application/json",
			dataType: 'json',
			complete: function(jqXHR, textStatus) {
				if(jqXHR.status == 200) newElement(newtagid, newsong);
				else alert('Unable to add new: ' + jqXHR.responseJSON["result"]);
			}		
		});
	}
});

// END
});
