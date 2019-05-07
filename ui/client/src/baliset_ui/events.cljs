(ns baliset-ui.events
  (:require [re-frame.core :as rf]
            [ajax.core :as ajax]))

(defn local-route [route]
  ;; (str (.-location js/window) route)
  (str "http://192.168.0.104:3001/" route))

(rf/reg-event-db
 :initialize-db
 (fn [db _]
   (assoc db :node-metadata {})))

(defonce db-init (rf/dispatch-sync [:initialize-db]))

(rf/reg-event-db
 :load-node-metadata
 (fn [db [_ result]]
   (let [node-map (reduce (fn [acc node-data]
                            (assoc acc (node-data "name") node-data))
                          {}
                          result)]
     (assoc db :node-metadata node-map))))

(rf/reg-event-db
 :todo
 (fn [db [_ result]]
   (println "uhoh failure")
   db))

(rf/reg-event-fx
 :request-node-metadata
 (fn [{:keys [db]} _]
   {:http-xhrio {:method :get
                 :uri (local-route "node_metadata")
                 :response-format (ajax/json-response-format {:keywords? false})
                 :on-failure [:todo] ;;TODO
                 :on-success [:load-node-metadata]}}))
