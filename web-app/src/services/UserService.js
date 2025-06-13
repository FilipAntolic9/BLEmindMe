import backendClient from "../api/axios.js";
import {AxiosError} from "axios";

export const getAllTenantUsers = async (pageSize, page) => {
    try {
        const response = await backendClient.get(`/users?pageSize=${pageSize}&page=${page}`);
        return {
            ...response.data,
            data: response.data.data.filter(user => user.authority === 'CUSTOMER_USER')
        };
    } catch (error) {
        if (error instanceof AxiosError) {
            throw new Error(error?.response?.data?.message);
        }
        throw error;
    }
};

export const assignDeviceToUser = async (customerId, deviceId) => {
    try {
        const response = await backendClient.post(`/customer/${customerId}/device/${deviceId}`);
        return response.data;
    } catch (error) {
        if (error instanceof AxiosError) {
            throw new Error(error?.response?.data?.message);
        }
        throw error;
    }
};