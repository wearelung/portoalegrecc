/**
 * jsii - JavaScript Inheritance Implementation
 * Copyright (c) 2009 Eduardo Nunes (http://e-nunes.com.br), Otávio Avila (http://otavioavila.com)
 * Licensed under GNU Lesser General Public License
 *
 * Inspired in http://ejohn.org/blog/simple-javascript-inheritance/
 * 
 * @docs http://code.google.com/p/jsii/
 * @version 1.0.0
 * 
 */

(function(){
			
    var preventConstructor;
    
    Class = function(){};
    
	Class.prototype.__jsii = new Object();
	Class.prototype.__jsii.wasInheritedCalled = false;
    
    Class.extend = function(properties) {

        var eClass = this.prototype;
		var eJsii = eClass.__jsii;

        function nClass() {
            if (!preventConstructor && this.init) {
                this.init.apply(this, arguments);
            }
        }

        preventConstructor = true;
        
        nClass.prototype = new this();
        
        preventConstructor = false;

		nClass.prototype.__jsii = new Object();
		var nJsii = nClass.prototype.__jsii;
		nJsii.methods = {};
		nJsii.methodsIndex = {};
        nJsii.metadata = nClass;

        for (var methodName in eJsii.methods) {
			nJsii.methods[methodName] = [];
            
            for (var i = 0; i < eJsii.methods[methodName].length; i++) {
                nJsii.methods[methodName].push(eJsii.methods[methodName][i]);
            }
        }

        for (var property in properties) {
            if (typeof(properties[property]) == "function") {
                if (!nJsii.methods[property]) {
                    nJsii.methods[property] = [];
                }
                nJsii.methods[property].push(properties[property]);
            }
        }

        for (var methodName in nJsii.methods) {
            nJsii.methodsIndex[methodName] = nJsii.methods[methodName].length - 1;
        }
        
        for (var methodName in nJsii.methods) {
            var method = nJsii.methods[methodName];
            
            var execute = function(method, methodName) {
                return function() {
                    var oldIndex = this.__jsii.methodsIndex[methodName];

                    if (this.__jsii.wasInheritedCalled) {
						if (this.__jsii.methodsIndex[methodName] > 0) {
                        	this.__jsii.methodsIndex[methodName]--;
						 } else {
							throw new Error('The method ' + methodName + ' doesn\'t exist.');
						}
                    }

                    while (this.__jsii.methodsIndex[methodName] > 0 && !method[this.__jsii.methodsIndex[methodName]]) {
                        this.__jsii.methodsIndex[methodName]--;
                    }
                    
                    if (!method[this.__jsii.methodsIndex[methodName]]) {
                        this.__jsii.methodsIndex[methodName] = oldIndex;
                        throw new Error("The method " + methodName + " doesn't exist.");
                    }
                    
                    this.__jsii.wasInheritedCalled = false;
                    
                    var result = method[this.__jsii.methodsIndex[methodName]].apply(this, arguments);
                    
                    this.__jsii.methodsIndex[methodName] = oldIndex;
                                    
                    return result;
                };
            };
            
            nClass.prototype[methodName] = execute(method, methodName);
        }

        nClass.prototype.inherited = function() {
            this.__jsii.wasInheritedCalled = true;
            return this;
        };

        nClass.constructor = nClass;
        nClass.extend = this.extend;

        return nClass;
    };
})();