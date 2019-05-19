(ns baliset-ui.websocket
  (:require [re-frame.core :as rf]))

(def port "3002")

(defonce sock (atom nil))

(defn handle-message [e]
  (let [msg (.parse js/JSON (.-data e))]
    (case (.-route msg)
      "/app_state"
      (rf/dispatch [:app-state msg])
      "/node/added"
      (rf/dispatch [:db-add-node (.-node_id msg) (.-type msg) (.-x msg) (.-y msg)])
      "/node/connected"
      (rf/dispatch [:db-connect-node (.-out_node_id msg) (.-outlet_idx msg)
                    (.-in_node_id msg) (.-inlet_idx msg)])
      "/node/disconnected"
      (rf/dispatch [:db-disconnect-node (.-out_node_id msg) (.-outlet_idx msg)
                    (.-in_node_id msg) (.-inlet_idx msg)])
      "/node/moved"
      (rf/dispatch [:move-node (.-node_id msg) (.-x msg) (.-y msg)])
      (println "unrecognized msg: " msg))))

(defn connect []
  (when-not @sock
    (let [ws (js/WebSocket. (str "ws://" (.-hostname (.-location js/window)) ":" port))]
      (.addEventListener ws "message" handle-message)
      (reset! sock ws))))

(defn close []
  (.close @sock)
  (reset! sock nil))

(rf/reg-fx
 :ws-add-node
 (fn [[node-type x y]]
   (.send @sock (.stringify js/JSON #js {:route "/node/add" :type node-type :x x :y y}))))

(rf/reg-fx
 :ws-move-node
 (fn [[id x y]]
   (.send @sock (.stringify js/JSON #js {:route "/node/move" :node_id id :x x :y y}))))

(rf/reg-fx
 :ws-connect
 (fn [[out-node-id outlet-idx in-node-id inlet-idx]]
   (.send @sock (.stringify js/JSON #js {:route "/node/connect"
                                         :out_node_id out-node-id
                                         :outlet_idx outlet-idx
                                         :in_node_id in-node-id
                                         :inlet_idx inlet-idx}))))

(rf/reg-fx
 :ws-disconnect
 (fn [[out-node-id outlet-idx in-node-id inlet-idx]]
   (.send @sock (.stringify js/JSON #js {:route "/node/disconnect"
                                         :out_node_id out-node-id
                                         :outlet_idx outlet-idx
                                         :in_node_id in-node-id
                                         :inlet_idx inlet-idx}))))

(comment
  (close)
  (connect)
  (rf/dispatch [:clicked-add-node "dac" (rand-int 500) (rand-int 500)])
  (rf/dispatch [:clicked-add-node "sin" (rand-int 500) (rand-int 500)])
  (rf/dispatch [:clicked-add-node "adc" (rand-int 500) (rand-int 500)])
  (rf/dispatch [:clicked-add-node "delay" (rand-int 500) (rand-int 500)])
  (rf/dispatch [:clicked-add-node "looper" (rand-int 500) (rand-int 500)])
  )
