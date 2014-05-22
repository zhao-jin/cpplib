// Copyright 2010 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

/**
 * @fileoverview Render form appropriate for RPC method.
 * @authors rafek@google.com (Rafe Kaplan)
 *          ericliu@tencent.com (Liu Yongsong)
 *          phongchen@tencent.com (Chen Feng)
 */


var FORM_VISIBILITY = {
    SHOW_FORM: 'Show Form',
    HIDE_FORM: 'Hide Form'
};


var FIELD_LABEL = {
    OPTIONAL: 1,
    REQUIRED: 2,
    REPEATED: 3
};


var objectId = 0;

function generateObjectId() {
    return objectId++;
}


var FIELD_TYPE = {
    INT32: 1,
    INT64: 2,
    UINT32: 3,
    UINT64: 4,
    DOUBLE: 5,
    FLOAT: 6,
    BOOL: 7,
    ENUM: 8,
    STRING: 9,
    MESSAGE: 10,
    BYTES: 11
};


var FIELD_TYPE_NAME = [
    '',
    'int32',
    'int64',
    'uint32',
    'uint64',
    'double',
    'float',
    'bool',
    'enum',
    'string',
    'message',
    'bytes',
];


var LIMITS = {
    INT32_MIN:  -2147483648,
    INT32_MAX:   2147483647,
    UINT32_MAX:  4294967295,
    FLOAT_MIN:  -3.40282346638528859812e+38,
    FLOAT_MAX:   3.40282346638528859812e+38,
    INT64_MIN: '-9223372036854775808',
    INT64_MAX:  '9223372036854775807',
    UINT64_MAX: '18446744073709551615'
};


function parseInterger32(input, reg, min, max) {
    var v = $.trim(input.val());
    if (v.length == 0)
        throw 'is empty';

    v = v.toLowerCase();
    if (!reg.test(v)) {
        throw 'is invalid';
    }

    if (!isFinite(v))
        throw 'out of range';

    var n = Number(v);
    if (n >= min && n <= max)
        return n;
    else
        throw 'out of range';
}

function parseInterger64(input, reg, max, hex_max, min) {
    var v = $.trim(input.val());
    if (v.length == 0)
        throw 'is empty';

    v = v.toLowerCase();
    if (!reg.test(v)) {
        throw 'is invalid';
    }

    if (!isFinite(v))
        throw 'out of range';

    if (v.charAt(0) == '-') {
        if (v.length > min.length || (v.length == min.length && v > min))
            throw 'out of range';
    } else {
        if (v.charAt(0) == '0' && v.charAt(1) == 'x') {
            if (v.length > 18 || (v.length == 18 && v > hex_max))
                throw 'out of range';
        } else {
            if (v.length > max.length || (v.length == max.length && v > max))
                throw 'out of range';
        }
    }

    // Javascript不支持64长整数，转成Number会有精度损失
    // 因此，在json中通过字符串格式传输到64长整数以保证精度
    // return Number(v);
    return v;
}

var TypeParser = [];

TypeParser[FIELD_TYPE.INT32] = function (input) {
    return parseInterger32(input,
            /^(?:0x[0-9a-f]+|-?\d+)$/,
            LIMITS.INT32_MIN,
            LIMITS.INT32_MAX);
};

TypeParser[FIELD_TYPE.INT64] = function (input) {
    return parseInterger64(input,
            /^(?:0x[0-9a-f]+|-?\d+)$/,
            LIMITS.INT64_MAX,
            '0x7fffffffffffffff',
            LIMITS.INT64_MIN);
};

TypeParser[FIELD_TYPE.UINT32] = function (input) {
    return parseInterger32(input,
            /^(?:0x[0-9a-f]+|\d+)$/,
            0,
            LIMITS.UINT32_MAX);
};

TypeParser[FIELD_TYPE.UINT64] = function (input) {
    return parseInterger64(input,
            /^(?:0x[0-9a-f]+|\d+)$/,
            LIMITS.UINT64_MAX,
            '0xffffffffffffffff');
};

TypeParser[FIELD_TYPE.DOUBLE] = function (input) {
    var v = $.trim(input.val());
    if (v.length == 0)
        throw 'is empty';

    if (!isFinite(v))
        throw 'out of range';

    return Number(v);
};

TypeParser[FIELD_TYPE.FLOAT] = function (input) {
    var n = TypeParser[FIELD_TYPE.DOUBLE](input);
    if (n >= LIMITS.FLOAT_MIN && n <= LIMITS.FLOAT_MAX)
        return n;
    else
        throw 'out of range';
};

TypeParser[FIELD_TYPE.BOOL] = function (input) {
    return input[0].checked;
};

TypeParser[FIELD_TYPE.ENUM] = function (input) {
    var n = parseInt(input.val());
    if (isNaN(n))
        throw 'is not selected.';
    return n;
};

TypeParser[FIELD_TYPE.STRING] = function (input) {
    // return encodeURIComponent(input.val());
    return input.val();
};

TypeParser[FIELD_TYPE.MESSAGE] = null;

TypeParser[FIELD_TYPE.BYTES] = function (input) {
    // TODO(ericliu) 非ASCII码字符以percent编码形式表示
    return input.val();
};


/**
 * Data structure used to represent a form to data element.
 * @param {Object} field Field descriptor that form element represents.
 * @param {Object} container Element that contains field.
 * @return {FormElement} New object representing a form element.  Element
 *     starts enabled.
 * @constructor
 */
function FormElement(field, container) {
    this.field = field;
    this.container = container;
    this.enabled = true;
}


/**
 * Display error message in error panel.
 * @param {string} message Message to display in panel.
 */
function error(message) {
    $('<div>').appendTo($('#error-messages')).html('<pre>' + message + '</pre>');
}


/**
 * Display request errors in error panel.
 * @param {object} XMLHttpRequest object.
 */
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

    send_button.disabled = false;
}


/**
 * Send JSON RPC to remote method.
 * @param {string} service_name service full name.
 * @param {string} method Name of method to invoke.
 * @param {Object} request Message to send as request.
 * @param {function} on_success Function to call upon successful request.
 */
function sendRequest(service_name, method_name, response_format, request, onSuccess) {
    $.ajax({
        url: service_name + '.' + method_name + '?of=' + response_format,
        type: 'POST',
        contentType: 'application/json',
        data: $.toJSON(request),
        // 让jQuery根据http response的MIME自动识别应答格式
        // dataType: 'json',
        success: onSuccess,
        error: handleRequestError
    });
}


/**
 * Create callback that enables and disables field element when associated
 * checkbox is clicked.
 * @param {Element} checkbox Checkbox that will be clicked.
 * @param {FormElement} form Form element that will be toggled for editing.
 * @return Callback that is invoked every time checkbox is clicked.
 */
function toggleInput(checkbox, form) {
    return function() {
        var checked = checkbox.checked;
        if (checked) {
            if (!form.input)
                buildIndividualForm(form);
            form.enabled = true;
        } else {
            form.enabled = false;
        }
    };
}


/**
 * Build an enum field.
 * @param {FormElement} form Form to build element for.
 */
function buildEnumField(form) {
    form.descriptor = enumDescriptors[form.field.type_name];
    form.input = $('<select>').appendTo(form.display);

    $('<option>').appendTo(form.input).attr('value', '').text('Select enum');
    $.each(form.descriptor.values, function(index, enumValue) {
        option = $('<option>');
        option.appendTo(form.input).attr('value', enumValue.number).text(enumValue.name);
        if (enumValue.number == form.field.default_value) {
            option.attr('selected', 1);
        }
    });
}


/**
 * Build nested message field.
 * @param {FormElement} form Form to build element for.
 */
function buildMessageField(form) {
    form.table = $('<table border="1">').appendTo(form.display);
    buildMessageForm(form, messageDescriptors[form.field.type_name]);
    // Input is not used for message field, assign a value to avoid build it again
    form.input = true;
}


/**
 * Build boolean field.
 * @param {FormElement} form Form to build element for.
 */
function buildBooleanField(form) {
    form.input = $('<input type="checkbox">');
    form.input[0].checked = Boolean(form.field.default_value);
}


/**
 * Build string field.
 * @param {FormElement} form Form to build element for.
 */
function buildStringField(form) {
    form.input = $('<textarea>');
    form.input.attr('title', 'Hint: value will always be transfered as UTF-8 encoding.');

    try {
        form.input.attr('value', decodeURIComponent(form.field.default_value || ''));
    } catch (ex) {
        form.input.attr('value', form.field.default_value || '');
    }
}


/**
 * Build text field.
 * @param {FormElement} form Form to build element for.
 */
function buildTextField(form) {
    form.input = $('<input type="text">');

    switch (form.field.type) {
    case FIELD_TYPE.INT32:
    case FIELD_TYPE.INT64:
    case FIELD_TYPE.UINT32:
    case FIELD_TYPE.UINT64:
        form.input.attr('title', 'Hint: you can also use "0x" prefix for hex value.');
        break;
    case FIELD_TYPE.FLOAT:
    case FIELD_TYPE.DOUBLE:
        form.input.attr('title', 'Hint: you can also use scientific notation.');
        break;
    case FIELD_TYPE.BYTES:
        form.input.attr('title', 'Hint: you should use %XX encoding for non-ASCII bytes.');
        break;
    }

    try {
        form.input.attr('value', decodeURIComponent(form.field.default_value || ''));
    } catch (ex) {
        form.input.attr('value', form.field.default_value || '');
    }
}

function inputChanged(form) {
    return function() {
        if ((form.field.type == FIELD_TYPE.STRING ||
            form.field.type == FIELD_TYPE.BYTES) &&
            form.input.val().length == 0) {
            return;
        }

        form.checkbox.attr("checked", form.input.val().length > 0);
        form.checkbox.change();
    }
}

/**
 * Build individual input element.
 * @param {FormElement} form Form to build element for.
 */
function buildIndividualForm(form) {
    if (form.field.type == FIELD_TYPE.ENUM) {
        buildEnumField(form);
    } else if (form.field.type == FIELD_TYPE.MESSAGE) {
        buildMessageField(form);
    } else if (form.field.type == FIELD_TYPE.BOOL) {
        buildBooleanField(form);
    } else if (form.field.type == FIELD_TYPE.STRING) {
        buildStringField(form);
    } else {
        buildTextField(form);
    }

    if (form.field.type != FIELD_TYPE.MESSAGE && form.checkbox) {
        // 同时处理keyup和chanage事件是为了解决浏览器的兼容性
        form.input.keyup(inputChanged(form));
        form.input.change(inputChanged(form));
    }
    form.display.append(form.input);
}


/**
 * Deletion of repeated items.  Needs to delete from DOM and
 * also delete from form model.
 * @param {FormElement} form Repeated form element to delete item for.
 */
function delRepeatedFieldItem(form) {
    form.container.remove();
    var index = $.inArray(form, form.parent.fields);
    form.parent.fields.splice(index, 1);
}


/**
 * Add repeated field.  This function is called when an item is added
 * @param {FormElement} form Repeated form element to create item for.
 */
function addRepeatedFieldItem(form) {
    var row = $('<tr>').appendTo(form.display);
    subForm = new FormElement(form.field, row);
    subForm.parent = form;
    form.fields.push(subForm);
    buildFieldForm(subForm, false);
}


/**
 * Build repeated field.  Contains a button that can be used for adding new
 * items.
 * @param {FormElement} form Form to build element for.
 */
function buildRepeatedForm(form) {
    form.fields = [];
    form.display = $('<table border="1" width="100%">').appendTo(form.container);
    var header_row = $('<tr>').appendTo(form.display);
    var header = $('<td colspan="3">').appendTo(header_row);
    var add_button = $('<button>').text('+').appendTo(header);
    add_button.attr('title', 'Hint: add a new repeated item.');
    add_button.click(function() {
        addRepeatedFieldItem(form);
    });
}


/**
 * Build a form field.  Populates form content with values required by
 * all fields.
 * @param {FormElement} form Repeated form element to create item for.
 * @param allowRepeated {Boolean} Allow display of repeated field.  If set to
 *     to true, will treat repeated fields as individual items of a repeated
 *     field and render it as an individual field.
 */
function buildFieldForm(form, allowRepeated) {
    // All form fields are added to a row of a table.
    var inputData = $('<td>');

    // Set name.
    if (allowRepeated) {
        var nameData = $('<td>');
        var field_type_name;
        if (form.field.type == FIELD_TYPE.MESSAGE || form.field.type == FIELD_TYPE.ENUM) {
            field_type_name = form.field.type_name;
        } else {
            field_type_name = FIELD_TYPE_NAME[form.field.type];
        }
        var html = ' <b>' + form.field.name + '</b>';
        if (form.field.label == FIELD_LABEL.REQUIRED) {
            html += '<font color="red">*</font>';
        }
        html += ' :' + field_type_name;
        if (form.field.label == FIELD_LABEL.REPEATED) {
            html += '[]';
        }
        nameData.html(html);
        form.container.append(nameData);
    }

    // Set input.
    form.repeated = form.field.label == FIELD_LABEL.REPEATED;
    if (allowRepeated && form.repeated) {
        inputData.attr('colspan', '2');
        buildRepeatedForm(form);
    } else {
        if (!allowRepeated) {
            inputData.attr('colspan', '2');
        }

        form.display = $('<div>');

        var controlData = $('<td>');
        if (form.field.label != FIELD_LABEL.REQUIRED && allowRepeated) {
            form.enabled = false;
            var checkbox_id = 'checkbox-' + generateObjectId();
            var checkbox = $('<input id="' + checkbox_id + '" type="checkbox">').appendTo(controlData);
            checkbox.attr('title', 'Hint: check this checkbox to enable this optional field.');
            checkbox.change(toggleInput(checkbox[0], form));
            form.checkbox = checkbox;
        } else {
            if (form.repeated) {
                var del_button = $('<button>').text('-').appendTo(controlData);
                del_button.attr('title', 'Hint: delete the item in this row.');
                del_button.click(function() {
                    delRepeatedFieldItem(form);
                });
            }
        }

        buildIndividualForm(form);
        form.container.append(controlData);
    }

    inputData.append(form.display);
    form.container.append(inputData);
}


/**
 * Top level function for building an entire message form.  Called once at form
 * creation and may be called again for nested message fields.  Constructs a
 * a table and builds a row for each sub-field.
 * @params {FormElement} form Form to build message form for.
 */
function buildMessageForm(form, messageType) {
    form.fields = [];
    form.descriptor = messageType;
    if (messageType.fields) {
        $.each(messageType.fields, function(index, field) {
            var row = $('<tr>').appendTo(form.table);
            var fieldForm = new FormElement(field, row);
            fieldForm.parent = form;
            buildFieldForm(fieldForm, true);
            form.fields.push(fieldForm);
        });
    }
}


/**
 * HTML Escape a string
 */
function htmlEscape(value) {
    if (typeof(value) == "string") {
        return value
            .replace(/&/g, '&amp;')
            .replace(/>/g, '&gt;')
            .replace(/</g, '&lt;')
            .replace(/"/g, '&quot;')
            .replace(/'/g, '&#39;')
            .replace(/ /g, '&nbsp;');
    } else {
        return value;
    }
}


/**
 * JSON formatted in HTML for display to users.  This method recursively calls
 * itself to render sub-JSON objects.
 * @param {Object} value JSON object to format for display.
 * @param {Integer} indent Indentation level for object being displayed.
 * @return {string} Formatted JSON object.
 */
function formatJSON(value, indent) {
    var indentation = '';
    var indentation_back = '';
    for (var index = 0; index < indent; ++index) {
        indentation = indentation + '&nbsp;&nbsp;&nbsp;&nbsp;';
        if (index == indent - 2)
            indentation_back = indentation;
    }
    var type = typeof(value);

    var result = '';

    if (type == 'object') {
        if (value.constructor === Array) {
            result += '[<br>';
            $.each(value, function(index, item) {
                result += indentation + formatJSON(item, indent + 1) + ',<br>';
            });
            result += indentation_back + ']';
        } else {
            result += '{<br>';
            $.each(value, function(name, item) {
                result += (indentation + htmlEscape(name) + ': ' +
                        formatJSON(item, indent + 1) + ',<br>');
            });
            result += indentation_back + '}';
        }
    } else {
        try {
            result += htmlEscape(decodeURIComponent(value));
        } catch (ex) {
            result += htmlEscape(value);
        }
    }
    return result;
}


/**
 * Construct array from repeated form element.
 * @param {FormElement} form Form element to build array from.
 * @return {Array} Array of repeated elements read from input form.
 */
function fromRepeatedForm(form) {
    var values = [];
    $.each(form.fields, function(index, subForm) {
        values.push(fromIndividualForm(subForm));
    });
    return values;
}

function getFieldFullname(form) {
    full_name = form.field.name;
    while(typeof form.parent !== "undefined" && form.parent.field != null) {
        full_name = form.parent.field.name + '.' + full_name;
        form = form.parent;
    }
    return full_name;
}

var first_failed_input;
var failed_messages;

/**
 * Construct value from individual form element.
 * @param {FormElement} form Form element to get value from.
 * @return {string, Float, Integer, Boolean, object} Value extracted from
 *     individual field.  The type depends on the field variant.
 */
function fromIndividualForm(form) {
    if (form.field.type == FIELD_TYPE.MESSAGE)
        return fromMessageForm(form);

    try {
        return TypeParser[form.field.type](form.input);
    } catch (ex) {
        failed_messages.push(getFieldFullname(form) + ' ' + ex + '.');
        if (!first_failed_input)
            first_failed_input = form.input;
        return null;
    }
}


/**
 * Extract entire message from a complete form.
 * @param {FormElement} form Form to extract message from.
 * @return {Object} Fully populated message object ready to transmit
 *     as JSON message.
 */
function fromMessageForm(form) {
    var message = {};
    $.each(form.fields, function(index, subForm) {
        if (subForm.enabled) {
            var subMessage = undefined;
            if (subForm.field.label == FIELD_LABEL.REPEATED) {
                subMessage = fromRepeatedForm(subForm);
            } else {
                subMessage = fromIndividualForm(subForm);
            }
            message[subForm.field.name] = subMessage;
        }
    });

    return message;
}


/**
 * Send form as an RPC.  Extracts message from root form and transmits to
 * originating poppy server.  Response is formatted as JSON and displayed
 * to user.
 */
function sendForm() {
    send_button = this;
    send_button.disabled = true;

    $('#error-messages').empty();
    $('#form-response').empty();

    first_failed_input = null;
    failed_messages = [];
    message = fromMessageForm(root_form);
    if (failed_messages.length > 0) {
        error("<h3>Input Error:</h3>" + failed_messages.join('\n'));
        first_failed_input.focus();
        first_failed_input.select();
        send_button.disabled = false;
        return;
    }

    var req_enc = root_form.request_encoding.val();
    var resp_enc = root_form.response_encoding.val();
    var qs = '';
    if (req_enc != '') qs += '&ie=' + req_enc;
    if (resp_enc == '') {
        qs += '&oe=' + req_enc;
    } else {
        qs += '&oe=' + resp_enc;
    }
    if (root_form.use_beautiful.attr("checked"))
        qs += '&b=1';

    sendRequest('../../rpc/' + serviceName, methodName, 'x-proto' + qs, message, function(response) {
        send_button.disabled = false;
        $('#form-response').html('<h3>Response:</h3><pre>' + response + '</pre>');
        if (root_form.fields.length > 0)
            hideForm();
    });
}


/**
 * Reset form to original state.  Deletes existing form and rebuilds a new
 * one from scratch.
 * @param {Object} response ajax response.
 */
function resetForm() {
    var panel = $('#form-panel');

    panel.empty();

    function formGenerationError(message) {
        error(message);
        panel.html('<div class="error-message">' +
                   'There was an error generating the service form' +
                   '</div>');
    }

    requestType = messageDescriptors[requestTypeName];
    if (!requestType) {
        formGenerationError('No such message-type: ' + requestTypeName);
        return;
    }

    if (requestType.fields)
        requestType.fields.sort(function (field1, field2) {
                                    return field1.number - field2.number;
                                });

    $('<div>').appendTo(panel).html('<br/>Request message type is '
                                    + requestTypeName + '<br/><br/>');

    var root = $('<table border="2">').appendTo(panel);

    root_form = new FormElement(null, null);
    root_form.table = root;
    buildMessageForm(root_form, requestType);
    $('<br>').appendTo(panel);
    buildEncodingForm(panel);
    $('<br>').appendTo(panel);
    $('<br>').appendTo(panel);
    $('<button>').appendTo(panel).text('Send Request').click(sendForm);
    if (requestType.fields)
        $('<button>').appendTo(panel).text('Reset').click(resetForm);
    else
        $('#form-expander').hide();
}


/**
 * Build encoding selection.
 * @param {panel} container for display.
 */
function buildEncodingForm(panel) {
    $('<label>').appendTo(panel).text('request encoding');

    var request_enc = $('<select>').appendTo(panel);
    root_form.request_encoding = request_enc;

    var option = $('<option>');
    option.appendTo(request_enc).attr('value', 'utf-8').text('UTF-8');
    option.attr('selected', 1);

    $('<option>').appendTo(request_enc).attr('value', 'gbk').text('GBK');
    $('<option>').appendTo(request_enc).attr('value', 'big5').text('BIG5');

    $('<label>').appendTo(panel).text(' response encoding');

    var response_enc = $('<select>').appendTo(panel);
    root_form.response_encoding = response_enc;

    option = $('<option>');
    option.appendTo(response_enc).attr('value', '').text('Same as request');
    option.attr('selected', 1);

    $('<option>').appendTo(response_enc).attr('value', 'utf-8').text('UTF-8');
    $('<option>').appendTo(response_enc).attr('value', 'gbk').text('GBK');
    $('<option>').appendTo(response_enc).attr('value', 'big5').text('BIG5');

    $('<br>').appendTo(panel);
    $('<br>').appendTo(panel);
    $('<label>').appendTo(panel).text(' ');
    root_form.use_beautiful =
        $('<input type="checkbox">').appendTo(panel);
    root_form.use_beautiful.attr("checked", true);
    $('<label>').appendTo(panel).text('use beautiful text format for response');
}


/**
 * Hide main RPC form from user.  The information in the form is preserved.
 * Called after RPC to server is completed.
 */
function hideForm() {
    var expander = $('#form-expander');
    var formPanel = $('#form-panel');
    formPanel.hide();
    expander.text(FORM_VISIBILITY.SHOW_FORM);
}


/**
 * Toggle the display of the main RPC form.  Called when form expander button
 * is clicked.
 */
function toggleForm() {
    var expander = $('#form-expander');
    var formPanel = $('#form-panel');
    if (expander.text() == FORM_VISIBILITY.HIDE_FORM) {
        hideForm();
    } else {
        formPanel.show();
        expander.text(FORM_VISIBILITY.HIDE_FORM);
    }
}


/**
 * Create form.
 */
function createForm() {
    sendRequest(
        '../../rpc/' + registryName,
        'GetMethodInputTypeDescriptors',
        'json',
        {method_full_name: serviceName + "." + methodName},
        function(response) {
            requestTypeName = response.request_type;
            messageDescriptors = {};
            $.each(response.message_types, function(index, message_type) {
                messageDescriptors[message_type.name] = message_type;
            });
            enumDescriptors = {};
            // 下面的条件判断等价于表达式 (typeof response.enum_types !== "undefined")
            if (response.enum_types) {
                $.each(response.enum_types, function(index, enum_type) {
                    enumDescriptors[enum_type.name] = enum_type;
                });
            }
            $('#form-expander').click(toggleForm);
            resetForm();
        });
}


/**
 * Display available services and their methods.
 * @param {Array} all service descriptors.
 */
function showMethods() {
    var methodSelector = $('#method-selector');
    $.each(serviceDescriptors, function(index, service) {
        methodSelector.append(service.name);
        var block = $('<blockquote>').appendTo(methodSelector);
        $.each(service.methods, function(index, method) {
            var url = ('form/' + service.name + '.' + method.name);
            var label = method.name;
            $('<a>').attr('href', url).text(label).appendTo(block);
            $('<br>').appendTo(block);
        });
    });
}


/**
 * Load all services from poppy registry service.  When services are
 * loaded, will then show all services and methods from the server.
 */
function loadServices() {
    sendRequest(
        '../rpc/' + registryName,
        'GetAllServiceDescriptors',
        'json',
        {},
        function(response) {
            serviceDescriptors = response.services;
            showMethods();
        });
}
