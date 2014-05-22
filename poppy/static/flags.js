// Copyright 2012, Tencent Inc.
// Author: Simon Wang <simonwang@tencent.com>

/*
 * modify flag object
 */
var FlagMessage = {
};

/*
 * show modify flag result message on page
 */
function showMessage(message) {
    $('<div>').appendTo($('#error-messages')).html('<pre>' + message + '</pre>');
}

function bindEvent() {
    $('.newvalue').bind('keyup change', inputChanged());
}

function inputChanged() {
    return function() {
        var value = $(this).val();
        var tr = $(this).parent().parent().parent();
        var td = $("#modify_flag", tr);
        var checkbox = $("input", td);
        checkbox.attr("checked", value.length > 0);
        checkbox.change();
    }
}

function handleSuccess(response) {
    if ($.isEmptyObject(response)) {
        showMessage("modify flag ok, please see blue row");
        for (var obj in FlagMessage) {
            var name = obj;
            $("tr").each(function(i, tr) {
                if ($("td.name", tr).text() == name) {
                    $(this).attr("style", "color:blue");
                    $("td.current", tr).text($.trim($("td input:text", tr).val()));
                    $("td input:text", tr).val("");
                    $("td input:checkbox", tr).attr("checked", false);
                    $("td input:checkbox", tr).change();
                }
            });
        }
    } else {
        showMessage("some flag modify error, please check green row");
        for (var obj in response) {
            showMessage(obj + " set to " + response[obj] + " is invalid, please check");
            $("tr").each(function(i, tr) {
                if ($("td.name", tr).text() == obj) {
                    $(this).attr("style", "color:green");
                }
            });
        }
    }
}

function handleRequestError(response) {
    var contentType = response.getResponseHeader('content-type');
    if (contentType == 'application/json') {
        var response_error = $.parseJSON(response.responseText);
        var error_message = response_error.error_message;
        if (error.state == 'APPLICATION_ERROR' && error.error_name) {
            error_message = error_message + ' (' + error.error_name + ')';
        }
    } else {
        error_message = response.status + ': ' + response.statusText;
        if (response.responseText.length > 0)
            error_message = error_message + ', ' + response.responseText;
    }

    error(error_message);
}

function sendRequest(handler_path, response_format, request, onSuccess) {
    $.ajax({
        url : handler_path + "?of=" + response_format,
        type: 'POST',
        contentType: 'application/json',
        data: $.toJSON(request),
        success: onSuccess,
        error: handleRequestError
    });
}

function fromTableRow() {
    var message = {}
    $("tr").each(function(i, tr) {
        var name = $("td.name", tr).text();
        var type = $("td.type", tr).text();
        var checked = $("td input:checkbox", tr).attr("checked");
        var new_value = $.trim($("td input:text", tr).val());
        if (checked) {
            message[name] = new_value;
        }
    });

    return message;
}

function submitModify() {
    $('#error-messages').empty();
    FlagMessage = fromTableRow();
    if ($.isEmptyObject(FlagMessage)) {
        showMessage("no flag need to modify");
        return;
    }
    sendRequest("./flags", "json", FlagMessage, handleSuccess);
}
