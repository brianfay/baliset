(ns baliset-ui.events
  (:require [re-frame.core :as rf]
            [ajax.core :as ajax]
            [goog.object :as goo]
            [clojure.set :as cset]))

(defn local-route [route]
  (str (goo/get js/window "location") route))

(rf/reg-event-db
 :initialize-db
 (fn [db _]
   ;;most of these defaults probably don't need to be here, this is kind of a poor man's schema
   (assoc db :node-metadata {}
             :connections #{}
             :drag-deltas {}
             :nodes {}
             :node-offset {}
             :pan [0 0]
             :pan-offset [0 0]
             :scale 1
             :pinch-scale 1
             :inlet-offset {}
             :outlet-offset {}
             :selected-io nil
             :selected-node nil
             :app-panel-expanded? false)))

(defonce db-init (rf/dispatch-sync [:initialize-db]))

(rf/reg-event-db
 :load-node-metadata
 (fn [db [_ result]]
   (assoc db :node-metadata result)))

(rf/reg-event-fx
 :load-patch
 (fn [_ [_ patch-name]]
   {:ws-load-patch [patch-name]}))

(rf/reg-event-fx
 :save-patch
 (fn [{:keys [db]} _]
   (let [patch-save-name (:patch-save-name db)]
     (if (not= patch-save-name "")
       {:ws-save-patch [patch-save-name]}
       {}))))

(rf/reg-event-db
 :patch-saved
 (fn [db [_ patch-name]]
   (assoc db :patches (set (conj (:patches db) patch-name)))))

(rf/reg-event-db
 :patch-save-name
 (fn [db [_ patch-name]]
   (assoc db :patch-save-name patch-name)))

(rf/reg-event-db
 :app-state
 (fn [db [_ resp]]
   (-> db
       (assoc :nodes
              (reduce-kv
               (fn [m k v]
                 (assoc m (js/parseInt (name k)) v))
               {}
               (js->clj (goo/get resp "nodes") :keywordize-keys true))
              :connections (set (js->clj (goo/get resp "connections")))
              :patches (vec (goo/get resp "patches"))))))

(rf/reg-event-db
 :node-metadata-failure ;;TODO
 (fn [db [_ result]]
   db))

(rf/reg-event-db
 :reg-inlet-offset
 (fn [db [_ node-id idx offset-top offset-height]]
   (assoc-in db [:inlet-offset node-id idx] [offset-top offset-height])))

(rf/reg-event-db
 :unreg-inlet-offset
 (fn [db [_ node-id idx]]
   (update-in db [:inlet-offset node-id] dissoc idx)))

(rf/reg-event-db
 :reg-outlet-offset
 (fn [db [_ node-id idx offset-top offset-height offset-left offset-width]]
   (assoc-in db [:outlet-offset node-id idx] [offset-top offset-height offset-left offset-width])))

(rf/reg-event-db
 :unreg-outlet-offset
 (fn [db [_ node-id idx]]
   (update-in db [:outlet-offset node-id] dissoc idx)))

(rf/reg-event-db
 :clicked-add-btn
 (fn [db [_ node-name]]
   (if (= (:selected-add-btn db) node-name)
     (assoc db :selected-add-btn nil)
     (assoc db :selected-add-btn node-name))))

(rf/reg-event-db
 :clicked-minimized-app-panel
 (fn [db _]
   (assoc db :app-panel-expanded? true)))

(rf/reg-event-db
 :clicked-minimized-node-panel
 (fn [db _]
   (assoc db :node-panel-expanded? true)))

(rf/reg-event-db
 :close-node-panel
 (fn [db _]
   (assoc db :node-panel-expanded? nil)))

(rf/reg-event-db
 :clicked-expanded-app-panel
 (fn [db _]
   (assoc db
          :selected-add-btn nil
          :selected-detail nil
          :app-panel-expanded? false
          :patch-save-name nil)))

(rf/reg-event-db
 :clicked-nodes-btn
 (fn [db _]
   (if (= :nodes (:selected-detail db))
     (assoc db
            :selected-detail nil
            :selected-add-btn nil)
     (assoc db :selected-detail :nodes))))

(rf/reg-event-db
 :clicked-patches-btn
 (fn [db _]
   (assoc db :selected-detail
          (if (= :patches (:selected-detail db))
              nil
              :patches))))
;;cases
;;nothing selected
;; select inlet
;;inlet already selected
;; unselect
;;other inlet selected
;; select inlet
;;outlet already selected
;;  node id is same
;;    select inlet
;;  node id is different
;;    connect (but ideally check for cycles, maybe indicate with colors)

(rf/reg-event-fx
 :clicked-inlet
 (fn [{:keys [db]} [_ node-id inlet-idx]]
   (let [selected-io (:selected-io db)
         [in-or-out out-node-id outlet-idx] selected-io
         connecting? (and (= in-or-out :outlet) (not= node-id out-node-id))
         already-connected? (and connecting?
                                 (contains? (:connections db)
                                            [out-node-id outlet-idx node-id inlet-idx]))]
     (merge
      {:db (cond
             (= selected-io [:inlet node-id inlet-idx])
             (assoc db :selected-io nil)

             (or already-connected? connecting?)
             db

             :default
             (assoc db :selected-io [:inlet node-id inlet-idx]))}
      (cond already-connected?
            {:ws-disconnect [out-node-id outlet-idx node-id inlet-idx]}

            connecting?
            {:ws-connect [out-node-id outlet-idx node-id inlet-idx]})))))

(rf/reg-event-fx
 :clicked-outlet
 (fn [{:keys [db]} [_ node-id outlet-idx]]
   (let [selected-io (:selected-io db)
         [in-or-out in-node-id inlet-idx] selected-io
         connecting? (and (= in-or-out :inlet) (not= node-id in-node-id))
         already-connected? (and connecting?
                                 (contains? (:connections db)
                                            [node-id outlet-idx in-node-id inlet-idx]))]
     (merge {:db (cond
                   (= selected-io [:outlet node-id outlet-idx])
                   (assoc db :selected-io nil)

                   (or already-connected? connecting?)
                   db

                   :default
                   (assoc db :selected-io [:outlet node-id outlet-idx]))}
            (cond already-connected?
                  {:ws-disconnect [node-id outlet-idx in-node-id inlet-idx]}

                  connecting?
                  {:ws-connect [node-id outlet-idx in-node-id inlet-idx]})))))

(rf/reg-event-fx
 :clicked-app-div
 (fn [{:keys [db]} [_ x y]]
   (let [[pan-x pan-y] (mapv + (:pan db) (:pan-offset db))
         scale (:scale db)]
     (merge {}
            (cond (:selected-add-btn db)
                  {:ws-add-node [(:selected-add-btn db) (- (/ x scale) pan-x) (- (/ y scale) pan-y)]}

                  :default
                  {:db (assoc db
                              :selected-io nil
                              ;; :node-panel-expanded? nil
                              ;; :selected-node nil
                              )})))))


(rf/reg-event-db
 :db-add-node
 (fn [db [_ node-id node-type x y]]
   (assoc-in db [:nodes node-id] {:type node-type :x x :y y
                                  :controls (mapv #(get % "default")
                                                  (get-in db [:node-metadata node-type "controls"]))})))

(rf/reg-event-db
 :db-delete-node
 (fn [db [_ node-id]]
   (let [new-connections (set (filter (fn [conn] (and (not= node-id (nth conn 0))
                                                      (not= node-id (nth conn 2))))
                                      (:connections db)))]
     (-> db
         (update :nodes dissoc node-id)
         (assoc :connections new-connections)))))

(rf/reg-event-db
 :db-connect-node
 (fn [db [_ out-node-id outlet-idx in-node-id inlet-idx]]
   (update db :connections cset/union #{[out-node-id outlet-idx in-node-id inlet-idx]})))

(rf/reg-event-db
 :db-disconnect-node
 (fn [db [_ out-node-id outlet-idx in-node-id inlet-idx]]
   (update db :connections disj [out-node-id outlet-idx in-node-id inlet-idx])))

(rf/reg-event-fx
 :request-node-metadata
 (fn [_ _]
   {:http-xhrio {:method :get
                 :uri (local-route "node_metadata")
                 :response-format (ajax/json-response-format {:keywords? false})
                 :on-failure [:node-metadata-failure]
                 :on-success [:load-node-metadata]}}))

;;TODO velocity based panning?

(rf/reg-event-db
 :drag-node
 (fn [db [_ id x y]]
   (let [scale (:scale db)]
     (-> db
         (assoc :recently-interacted-node id)
         (assoc-in [:node-offset id] [(/ x scale) (/ y scale)])))))

(rf/reg-event-db
 :clicked-node-header
 (fn [db [_ id]]
   (assoc db
          :selected-node id
          :node-panel-expanded? (if (= id (:selected-node db))
                                  (not (:node-panel-expanded? db))
                                  true))))

(rf/reg-event-fx
 :clicked-delete-node-btn
 (fn [{:keys [db]} _]
   {:ws-delete-node [(:selected-node db)]
    :db (assoc db :selected-node nil)}))

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
 :set-control
 (fn [db [_ node-id ctl-id ctl-val]]
   (assoc-in db [:nodes node-id :controls ctl-id] ctl-val)))

(rf/reg-event-fx
 :ctl-num-input.submit
 (fn [{:keys [db]} [_ node-id ctl-id ctl-val]]
   {:db (-> db
            (assoc-in [:nodes node-id :controls ctl-id] ctl-val)
            (update-in [:ctl-num-input.val node-id] dissoc ctl-id)
            (update-in [:ctl-num-input.err node-id] dissoc ctl-id))
    :ws-control-node [node-id ctl-id ctl-val]}))

(rf/reg-event-db
 :ctl-num-input.err
 (fn [db [_ node-id ctl-id err-msg]]
   (assoc-in db [:ctl-num-input.err node-id ctl-id] err-msg)))

(rf/reg-event-db
 :ctl-num-input.change
 (fn [db [_ node-id ctl-id ctl-val]]
   (-> db
       (assoc-in [:ctl-num-input.val node-id ctl-id] ctl-val)
       (update-in [:ctl-num-input.err node-id] dissoc ctl-id))))

(rf/reg-event-db
 :ctl-num-input.change-with-err
 (fn [db [_ node-id ctl-id ctl-val err-msg]]
   (-> db
       (assoc-in [:ctl-num-input.val node-id ctl-id] ctl-val)
       (assoc-in [:ctl-num-input.err node-id ctl-id] err-msg))))

(rf/reg-event-fx
 :drag-hslider
 (fn [{:keys [db]} [_ node-type node-id ctl-id delta width]]
   (let [ctl-metadata (nth (get-in db [:node-metadata node-type "controls"]) ctl-id)
         [min max] (or (get ctl-metadata "range") [0.0 1.0])
         old-ctl-value (get-in db [:nodes node-id :controls ctl-id])
         current-delta (or (get-in db [:drag-deltas node-id ctl-id]) 0)
         new-ctl-value (+ old-ctl-value (* (- max min) (/ (- delta current-delta) width)))]
     (cond (< max new-ctl-value)
           {:db (-> db
                    (assoc-in [:drag-deltas node-id ctl-id] delta)
                    (assoc-in [:nodes node-id :controls ctl-id] max)
                    (update-in [:ctl-num-input.val node-id] dissoc ctl-id)
                    (update-in [:ctl-num-input.err node-id] dissoc ctl-id))
            :ws-control-node [node-id ctl-id max]}

           (> min new-ctl-value)
           {:db (-> db
                    (assoc-in [:drag-deltas node-id ctl-id] delta)
                    (assoc-in [:nodes node-id :controls ctl-id] min)
                    (update-in [:ctl-num-input.val node-id] dissoc ctl-id)
                    (update-in [:ctl-num-input.err node-id] dissoc ctl-id))
            :ws-control-node [node-id ctl-id min]}

           :else
           {:db (-> db
                    (assoc-in [:drag-deltas node-id ctl-id] delta)
                    (assoc-in [:nodes node-id :controls ctl-id] new-ctl-value)
                    (update-in [:ctl-num-input.val node-id] dissoc ctl-id)
                    (update-in [:ctl-num-input.err node-id] dissoc ctl-id))
            :ws-control-node [node-id ctl-id new-ctl-value]}))))

(rf/reg-event-fx
 :trigger
 (fn [_ [_ node-id ctl-id ctl-val]]
   {:ws-control-node [node-id ctl-id ctl-val]}))

(rf/reg-event-db
 :finish-dragging-hslider
 (fn [db [_ node-type node-id ctl-id]]
   (update-in db [:drag-deltas node-id] dissoc ctl-id)))

(rf/reg-event-db
 :pan-camera
 (fn [db [_ x y]]
   (let [scale (:scale db)]
     (assoc db :pan-offset [(/ x scale) (/ y scale)]))))

(rf/reg-event-db
 :finish-pan-camera
 (fn [db _]
   (assoc db
          :pan (mapv + (:pan db) (:pan-offset db))
          :pan-offset [0 0])))

(rf/reg-event-db
 :scrolled-wheel
 (fn [db [_ delta-y client-x client-y]]
   (let [old-scale (:scale db)
         new-scale (+ old-scale (* 0.2 (* -1 (Math/sign delta-y))))
         scale-change (- new-scale old-scale)
         [pan-x pan-y] (:pan db)]
     (if (> new-scale 0.2)
       (assoc db
              :scale new-scale
              :pan [(+ pan-x (- (/ client-x new-scale) (/ client-x old-scale)))
                    (+ pan-y (- (/ client-y new-scale) (/ client-y old-scale)))])
       db))))

(rf/reg-event-db
 :pinched-app
 (fn [db [_ pinch-scale center]]
   (let [x (goo/get center "x")
         y (goo/get center "y")
         scale (:scale db)
         old-pinch-scale (:pinch-scale db)
         old-scale (+ (- old-pinch-scale 1) scale)
         new-scale (+ (- pinch-scale 1) scale)
         [pan-x pan-y] (:pan db)]
     (if (> new-scale 0.2)
       (assoc db
              :pinch-scale pinch-scale
              :pan [(+ pan-x (- (/ x new-scale) (/ x old-scale)))
                    (+ pan-y (- (/ y new-scale) (/ y old-scale)))])
       db))))

(rf/reg-event-db
 :stopped-pinching-app
 (fn [db _]
   (assoc db
          :scale (+ (- (:pinch-scale db) 1) (:scale db))
          :pinch-scale 1)))
