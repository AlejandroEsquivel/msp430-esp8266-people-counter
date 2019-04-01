'use strict';

/**
 * @ngdoc overview
 * @name tcpFrontendApp
 * @description
 * # tcpFrontendApp
 *
 * Main module of the application.
 */
angular
  .module('tcpFrontendApp', [
    'ngCookies',
    'ngRoute',
    'angular-websocket',
    'chart.js',
    'config'
  ])
  .config(function ($routeProvider) {
    $routeProvider
      .when('/', {
        templateUrl: 'views/main.html',
        controller: 'MainCtrl',
        controllerAs: 'main'
      })
      .otherwise({
        redirectTo: '/'
      });
  });
