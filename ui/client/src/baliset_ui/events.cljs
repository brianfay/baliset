(ns baliset-ui.events
  (:require [re-frame.core :as rf]
            [ajax.core :as ajax]))

(defn local-route [route]
  ;; (str (.-location js/window) route)
  (str "http://192.168.0.104:3001/" route))

(rf/reg-event-db
 :initialize-db
 (fn [db _]
   (assoc db :node-metadata {}
             :nodes {}
             :pan [0 0]
             :pan-offset [0 0])))

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
 :load-nodes
 (fn [db [_ resp]]
   (assoc db :nodes (js->clj (.-nodes resp) :keywordize-keys true))))

(comment
  (js->clj (.-nodes res) :keywordize-keys true)
  (:nodes db)
  )

(rf/reg-event-db
 :todo
 (fn [db [_ result]]
   db))

(rf/reg-event-fx
 :clicked-add-node
 (fn [cofx [_ node-type x y]]
   {;;TODO - optimistic update :db
    :ws-add-node [node-type x y]}))

(rf/reg-event-db
 :db-add-node
 (fn [db [_ node-id node-type x y]]
   (assoc-in db [:nodes node-id] {:type node-type :x x :y y})))

(rf/reg-event-fx
 :request-node-metadata
 (fn [_ _]
   {:http-xhrio {:method :get
                 :uri (local-route "node_metadata")
                 :response-format (ajax/json-response-format {:keywords? false})
                 :on-failure [:todo] ;;TODO
                 :on-success [:load-node-metadata]}}))

;;TODO velocity based panning?

(rf/reg-event-db
 :drag-node
 (fn [db [_ id x y]]
   (assoc-in db [:node-offset id] [x y])))

(rf/reg-event-fx
 :finish-drag-node
 (fn [{:keys [db]} [_ id]]
   (let [[x y] (get (:node-offset db) id)
         new-x (+ x (get-in db [:nodes id :x]))
         new-y (+ y (get-in db [:nodes id :y]))]
     {:db (-> db
              (assoc-in [:node-offset id] [0 0])
              (assoc-in [:nodes id :x] new-x)
              (assoc-in [:nodes id :y] new-y))
      :ws-move-node [id new-x new-y]})))

(rf/reg-event-db
 :move-node
 (fn [db [_ id x y]]
   (-> db
       (assoc-in [:nodes id :x] x)
       (assoc-in [:nodes id :y] y))))

(rf/reg-event-db
 :pan-camera
 (fn [db [_ x y]]
   (assoc db :pan-offset [x y])))

(rf/reg-event-db
 :finish-pan-camera
 (fn [db _]
   (assoc db
          :pan (mapv + (:pan db) (:pan-offset db))
          :pan-offset [0 0])))
