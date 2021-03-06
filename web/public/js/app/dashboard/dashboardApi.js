/**
 * Developer: Stepan Burguchev
 * Date: 9/27/2015
 * Copyright: 2009-2015 Stepan Burguchev®
 *       All Rights Reserved
 */

/* global define, require, Handlebars, Backbone, Marionette, $, _, Localizer */

define([
    './controllers/DashboardController'
], function (DashboardController) {
    'use strict';

    return {
        Controller: DashboardController
    };
});
