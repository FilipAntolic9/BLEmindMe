import backendClient from "../api/axios";
import { AxiosError } from "axios";
import {login} from "./AuthService.js";

const DEVICE_TYPES = {
    ESP: "c91cef10-43df-11f0-a544-db21b46190ed",
    TAG: "76042e60-4150-11f0-a544-db21b46190ed"
}

const TENANT_USERNAME = "luka.ivekovic@fer.hr";
const TENANT_PASSWORD = "Intstv123";

export const createDevice = async (name, type, macAdress, deviceType) => {
    try {
        let deviceProfileId = {
            id: DEVICE_TYPES[deviceType],
            entityType: "DEVICE_PROFILE"
        }

        const response = await backendClient.post('/device', {name, type, label: macAdress, deviceProfileId});
        return response.data;
    } catch (error) {
        if (error instanceof AxiosError) {
            throw new Error(error?.response?.data?.message);
        }
        throw error;
    }
};

export const getAllTenantDevices = async (pageSize, page) => {
    try {
        const loginResponse = await login(TENANT_USERNAME, TENANT_PASSWORD);

        const response = await backendClient.get(`/tenant/devices?pageSize=${pageSize}&page=${page}`, {
            skipInterceptor: true,
            headers: {
                'X-Authorization': 'Bearer ' + loginResponse?.token
            }
        });
        return response.data;
    } catch (error) {
        if (error instanceof AxiosError) {
            throw new Error(error?.response?.data?.message);
        }
        throw error;
    }
};

export const assignTagToEsp = async (espDeviceId, tagDeviceId) => {
    try {
        let from = {
            id: espDeviceId,
            entityType: "DEVICE"
        }

        let to = {
            id: tagDeviceId,
            entityType: "DEVICE"
        }

        const response = await backendClient.post('/relation', {from, to, type: 'Contains', typeGroup: 'COMMON'});
        return response.data;
    } catch (error) {
        if (error instanceof AxiosError) {
            throw new Error(error?.response?.data?.message);
        }
        throw error;
    }
};

export const getAllCustomerDevices = async (customerId, pageSize, page) => {
    try {
        const response = await backendClient.get(`/customer/${customerId}/devices?pageSize=${pageSize}&page=${page}`);
        return response.data;
    } catch (error) {
        if (error instanceof AxiosError) {
            throw new Error(error?.response?.data?.message);
        }
        throw error;
    }
};

export const getDevicesRelations = async (deviceId, direction) => {
    try {
        let parameters = {
            rootId: deviceId,
            rootType: "DEVICE",
            direction: direction,
            relationTypeGroup: "COMMON",
            fetchLastLevelOnly: false
        };

        let deviceTypes = ["ESP32", "Tags"];

        const response = await backendClient.post(`/devices`, {parameters: parameters, deviceTypes});
        return response.data;
    } catch (error) {
        if (error instanceof AxiosError) {
            throw new Error(error?.response?.data?.message);
        }
        throw error;
    }
};

export const getDeviceLightState = async (deviceId) => {
    try {
        const loginResponse = await login(TENANT_USERNAME, TENANT_PASSWORD);

        const response = await backendClient.get(`/plugins/telemetry/DEVICE/${deviceId}/values/attributes`, {
            skipInterceptor: true,
            headers: {
                'X-Authorization': 'Bearer ' + loginResponse?.token
            }
        });
        return response.data.filter(item => item.key.toLowerCase().startsWith('lightstate'));

    } catch (error) {
        if (error instanceof AxiosError) {
            throw new Error(error?.response?.data?.message);
        }
        throw error;
    }
};