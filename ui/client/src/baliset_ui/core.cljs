(ns ^:figwheel-hooks baliset-ui.core
  (:require [reagent.core :as reagent]
            [re-frame.core :as rf]
            [day8.re-frame.http-fx]
            [baliset-ui.views :as v]
            [baliset-ui.db :as db]
            [baliset-ui.events :as events]
            [baliset-ui.subs :as subs]))

(enable-console-print!)

(defn ^:export main
  []
  (rf/dispatch [:request-node-metadata])
  (reagent/render v/app
                  (.getElementById js/document "app")))

(defn ^:after-load reload []
  (reagent/render [v/app]
                  (.getElementById js/document "app")))

(defonce run-app! (main))
